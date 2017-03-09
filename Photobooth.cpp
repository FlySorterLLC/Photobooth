/* Copyright (c) 2016/2017, FlySorter LLC *
 *                                        *
 *                                        */ 

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <pylon/PylonIncludes.h>
#include <pylon/ImagePersistence.h>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;
using namespace Pylon;
using namespace std;

/* Here's what we do:
   - Find Arduino serial port. Open in, set up GPIOs.
   - Find dispenser serial port. Open it, put dispenser into USB mode.
   - Find servo controller USB port. Open it, configure controller.
   - Find two Basler cameras. Configure them.
   - Wait for command, then:
       - Move servos, dispense fly, move servos
       - Trigger stepper motor to rotate background into place
       - Trigger upper camera to capture N images
       - Trigger stepper motor to rotate background into place
       - Trigger lower camera to capture N images
       - Move servos, move fly to vial, move servos

*/

const char *servoCtrl = "/dev/ttyACM0";
const char *dispenser = "/dev/ttyACM2";
const char *arduino   = "/dev/ttyUSB0";

#define  INLET_GATE_OPEN   7100
#define  INLET_GATE_CLOSED 5400

#define OUTLET_GATE_OPEN   4500
#define OUTLET_GATE_CLOSED 6500

// NOTE: The Maestro's serial mode must be set to "USB Dual Port".

// Gets the position of a Maestro channel.
// See the "Serial Servo Commands" section of the user's guide.
int maestroGetPosition(int fd, unsigned char channel)
{
  unsigned char command[] = {0x90, channel};
  if(write(fd, command, sizeof(command)) == -1)
  {
    perror("error writing");
    return -1;
  }
  printf("mgp: wrote command %02X %02X.\n", command[0], command[1]);

  usleep(50000);
 
  unsigned char response[2];
  if(read(fd,response,2) != 2)
  {
    perror("error reading");
    return -1;
  }
  printf("mgp: read response %02X %02X.\n", response[0], response[1]);
 
  return response[0] + 256*response[1];
}
 
// Sets the target of a Maestro channel.
// See the "Serial Servo Commands" section of the user's guide.
// The units of 'target' are quarter-microseconds.
int maestroSetTarget(int fd, unsigned char channel, unsigned short target)
{
  unsigned char command[] = {0x84, channel, target & 0x7F, target >> 7 & 0x7F};
  if (write(fd, command, sizeof(command)) == -1)
  {
    perror("error writing");
    return -1;
  }
  return 0;
}

int openSerialPort(const char *dev) {

  int fd = open(dev, O_RDWR | O_SYNC );
  if (fd == -1)
  {
    perror(dev);
    return -1;
  } else {
    printf("FD opened (%d) for device: %s.\n", fd, dev);
  }
 
  struct termios options;
  if ( tcgetattr(fd, &options) < 0 ) {
    printf("Error reading termios option.\n");
    close(fd);
    return -1;
  }

  cfsetispeed(&options, B9600);
  cfsetospeed(&options, B9600);

  // 8N1
  options.c_cflag &= ~PARENB;
  options.c_cflag &= ~CSTOPB;
  options.c_cflag &= ~CSIZE;
  options.c_cflag |= CS8;
  // no flow control
  options.c_cflag &= ~CRTSCTS;

  //options.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset

  options.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
  options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
  options.c_oflag &= ~OPOST; // make raw

  // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
  options.c_cc[VMIN]  = 0;
  options.c_cc[VTIME] = 0;
  //options.c_cc[VTIME] = 20;
    
  tcsetattr(fd, TCSANOW, &options);
  if( tcsetattr(fd, TCSAFLUSH, &options) < 0) {
    perror("init_serialport: Couldn't set term attributes");
    close(fd);
    return -1;
  }

  printf("Options set.\n");

  return fd;
}

int serialport_read_until(int fd, char* buf, char until, int buf_max, int timeout)
{
    char b[1];  // read expects an array, so we give it a 1-byte array
    int i=0;
    do { 
        int n = read(fd, b, 1);  // read a char at a time
        if( n==-1) return -1;    // couldn't read
        if( n==0 ) {
            usleep( 1 * 1000 );  // wait 1 msec try again
            timeout--;
            if( timeout==0 ) { buf[i]=0; return -2; }
            continue;
        }
        // printf("sp_r_u: i=%d, n=%d b='%c'\n",i,n,b[0]); // debug
        buf[i] = b[0]; 
        i++;
    } while( b[0] != until && i < buf_max && timeout>0 );

    buf[i] = 0;  // null terminate the string
    return 0;
}

int main()
{

  printf("While serial ports are opening, set diffuser vane to block *lower* camera.\n");
  char replyString[100];
  int n;
  CInstantCamera upper, lower;

  PylonInitialize();

  int arduinoFD = openSerialPort(arduino);
  if ( arduinoFD == -1 ) {
    perror(arduino);
    return 1;
  } else {
    printf("Arduino FD opened: %d.\n", arduinoFD);
   
  }

  int servoFD = openSerialPort(servoCtrl);
  if (servoFD == -1)
  {
    perror(servoCtrl);
    return 1;
  } else {
    printf("Servo FD opened: %d.\n", servoFD);
  }

 
  int dispenserFD = openSerialPort(dispenser);
  if ( dispenserFD == -1 ) {
    perror(dispenser);
    return 1;
  } else {
    printf("Dispenser FD opened: %d.\n", dispenserFD);
   
  }
 
  usleep(1500000);

  printf("Current positions are %d and %d.\n", maestroGetPosition(servoFD, 0),
    maestroGetPosition(servoFD, 1));

  maestroSetTarget(servoFD, 0, INLET_GATE_OPEN);
  maestroSetTarget(servoFD, 1, OUTLET_GATE_CLOSED);

  // maestroSetTarget(servoFD, 0, INLET_GATE_CLOSED);
  // maestroSetTarget(servoFD, 1, OUTLET_GATE_CLOSED);
  // maestroSetTarget(servoFD, 1, OUTLET_GATE_CLOSED);

  n = write(dispenserFD, "I", 1);
  if ( n != 1 ) { perror("error writing to dispenser"); return 1; }

  usleep(1500000);

  n = serialport_read_until(dispenserFD, replyString, '\n', 100, 2000);
  if ( n == 0 ) {
    if ( strcmp(replyString, "ok\n") == 0 ) {
      printf("Dispenser initialized.\n");
    } else {
      printf("Expected 'ok' from dispenser, received: '%s'\n", replyString);
    }
    tcflush(dispenserFD, TCIOFLUSH);
  } else {
    perror("error reading from dispenser"); return 1;
  }

  n = write(arduinoFD, "A\n", 2);
  if ( n != 2 ) { perror("error writing to arduino"); return 1; }

  usleep(500000);

  n = serialport_read_until(arduinoFD, replyString, '\n', 100, 2000);
  if ( n == 0 ) {
    if ( strcmp(replyString, "A\n") == 0 ) {
      printf("Arduino initialized.\n");
    } else {
      printf("Expected 'A' from arduino, received: '%s'\n", replyString);
    }
  } else {
    perror("error reading from arduino"); return 1;
  }

  try
  {
    // Get the transport layer factory.
    CTlFactory& tlFactory = CTlFactory::GetInstance();

    // Get all attached devices and exit application if
    // two cameras aren't found
    DeviceInfoList_t devices;
    if ( tlFactory.EnumerateDevices(devices) != 2 )
    {
      cout << "Found " << devices.size() << " cameras." << endl;
        throw RUNTIME_EXCEPTION( "Did not find exactly two cameras.");
    }

    cout << "Device 0 name: " << devices[0].GetFriendlyName() << endl;
    cout << "Device 1 name: " << devices[1].GetFriendlyName() << endl;
    if ( strncmp( devices[0].GetFriendlyName(), "upper", 5) == 0 ) {
      cout << "Device 0 is upper." << endl;
      upper.Attach(tlFactory.CreateDevice( devices[0] ) ); upper.Open();
      lower.Attach(tlFactory.CreateDevice( devices[1] ) ); lower.Open();
    } else {
      cout << "Device 1 is upper." << endl;
      upper.Attach(tlFactory.CreateDevice( devices[1] ) ); upper.Open();
      lower.Attach(tlFactory.CreateDevice( devices[0] ) ); lower.Open();
    }


  } catch (const GenericException &e) {
    // Error handling.
    cerr << "An exception occurred." << endl
      << e.GetDescription() << endl;
    return 1;
  }
 

  // Now we're all set up.

  int keepDispensing = 1;
  while ( keepDispensing ) {
    n = write(dispenserFD, "F", 1);
    if ( n != 1 ) { perror("error writing to Dispenser"); return 1; }
    
    usleep(1500000);
 
    n = serialport_read_until(dispenserFD, replyString, '\n', 100, 2000);
    if ( n == 0 ) {
      if ( strcmp(replyString, "ok\n") == 0 ) {
        printf("Dispensing fly.\n");
      } else {
        printf("Expected 'ok' from dispenser, received: '%s'\n", replyString);
      }
      tcflush(dispenserFD, TCIOFLUSH);
    } else {
      perror("error reading from dispenser"); return 1;
    }

    // Now read status of dispenser (up to 20 s delay):
    //   f = fly dispensed
    //   t = timeout
    //   n = didn't detect fly at tip

    n = serialport_read_until(dispenserFD, replyString, '\n', 100, 20000);
    if ( n == 0 ) {
      if ( strcmp(replyString, "f\n") == 0 ) {
        printf("Dispensed fly.\n");
      } else if ( strcmp(replyString, "t\n") == 0 ) {
        printf("Timeout waiting for dispense.\n");
        keepDispensing = 0;
      } else if ( strcmp(replyString, "n\n") == 0 ) {
        printf("Dispensed fly but didn't see at detector.\n");
        keepDispensing = 0;
      } else {
        printf("Expected 'ok' from dispenser, received: '%s'\n", replyString);
      }
      tcflush(dispenserFD, TCIOFLUSH);
    } else {
      perror("error reading from dispenser"); return 1;
    }

    if ( keepDispensing == 1 ) {
      // Close the gate and take a picture or two!
      maestroSetTarget(servoFD, 0, INLET_GATE_CLOSED);

      CGrabResultPtr ptrGrabResult;
      char filename[100]; int imgCount;

      CImageFormatConverter fc;
      fc.OutputPixelFormat = PixelType_BGR8packed;
      CPylonImage image;

      upper.StartGrabbing(3);
      usleep(1000000);
      
      // Now spin the vanes


      lower.StartGrabbing(3);

      imgCount = 0;
      while ( upper.IsGrabbing() ) {
        upper.RetrieveResult(5000, ptrGrabResult,
          TimeoutHandling_ThrowException);
        if ( ptrGrabResult->GrabSucceeded()) {
          snprintf(filename, 100, "images/Upper%03d.png", imgCount++);
          cout << "Writing image to " << filename << endl;
          // Convert to OpenCV Mat
          fc.Convert(image, ptrGrabResult);
          Mat cimg(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(),
            CV_8UC3, (uint8_t *)image.GetBuffer());
          imwrite(filename, cimg);
        } else {
          cout << "Error: " << ptrGrabResult->GetErrorCode() << " "
               << ptrGrabResult->GetErrorDescription() << endl;
        }
      }

      imgCount=0;
      while ( lower.IsGrabbing() ) {
        lower.RetrieveResult(5000, ptrGrabResult,
          TimeoutHandling_ThrowException);
        if ( ptrGrabResult->GrabSucceeded()) {
          snprintf(filename, 100, "images/Lower%03d.png", imgCount++);
          cout << "Writing image to " << filename << endl;
          // Convert to OpenCV Mat
          fc.Convert(image, ptrGrabResult);
          Mat cimg(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(),
            CV_8UC3, (uint8_t *)image.GetBuffer());
          imwrite(filename, cimg);
        } else {
          cout << "Error: " << ptrGrabResult->GetErrorCode() << " "
               << ptrGrabResult->GetErrorDescription() << endl;
        }
      }

      // For now, be done.
      maestroSetTarget(servoFD, 1, OUTLET_GATE_OPEN);

      keepDispensing = 0;


    }

  }

  // Cleanup

  maestroSetTarget(servoFD, 0, INLET_GATE_OPEN);
  maestroSetTarget(servoFD, 1, OUTLET_GATE_OPEN);

  upper.Close();
  lower.Close();

  close(servoFD);
  close(dispenserFD);
  close(arduinoFD);

  return 0;
}
