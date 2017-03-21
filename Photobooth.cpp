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

#include "PhotoFuncs.h"

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
 
  usleep(1000000);

  printf("Current positions are %d and %d.\n", maestroGetPosition(servoFD, 0),
    maestroGetPosition(servoFD, 1));

  maestroSetTarget(servoFD, 0, INLET_GATE_OPEN);
  maestroSetTarget(servoFD, 1, OUTLET_GATE_CLOSED);

  if ( initDispenser(dispenserFD) != 0 ) {
    perror("error initializing dispenser"); return 1;
  }

  if ( enableLights(arduinoFD) != 0 ) {
    perror("error enabling lights"); return 1;
  }

  printf("Dispenser initialized, lights enabled.\n");
    
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

  printf("Cameras all set up.\n");
 

  // Now we're all set up.

  int keepDispensing = 1;

  while ( keepDispensing ) {
    printf("Dispensing fly.\n");

    usleep(100000);
    if ( dispenseFly(dispenserFD) != 0 ) {
      perror("error dispensing fly"); return 1;
    }

    usleep(100000);
    tcflush(dispenserFD, TCIOFLUSH);

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
        printf("After dispense, expected 'ok' from dispenser, received: '%s'\n", replyString);
      }
      tcflush(dispenserFD, TCIOFLUSH);
    } else {
      perror("error reading from dispenser"); return 1;
    }

    if ( keepDispensing == 1 ) {
      // Close the gate and take a picture or two!
      maestroSetTarget(servoFD, 0, INLET_GATE_CLOSED);
      usleep(1000000);

      CGrabResultPtr ptrGrabResult;
      char filename[100]; int imgCount;

      CImageFormatConverter fc;
      fc.OutputPixelFormat = PixelType_BGR8packed;
      CPylonImage image;

      upper.StartGrabbing(3);
      usleep(1000000);
      
      // Now spin the vanes
      if ( stepVanes(arduinoFD) != 0 ) {
        perror("error stepping vanes"); return 1;
      }

      usleep(1000000);

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

      // Spin the vanes back
      if ( stepVanes(arduinoFD) != 0 ) {
        perror("error stepping vanes"); return 1;
      }

      maestroSetTarget(servoFD, 1, OUTLET_GATE_OPEN);

      usleep(100000);

      if ( pumpOn(arduinoFD) != 0 ) {
        perror("error turning on pump"); return 1;
      }

      usleep(3000000);

      if ( pumpOff(arduinoFD) != 0 ) {
        perror("error turning on pump"); return 1;
      }

      keepDispensing = 0;

    }

  }

  // Cleanup
  stepperOff(arduinoFD);

  maestroSetTarget(servoFD, 0, INLET_GATE_OPEN);
  maestroSetTarget(servoFD, 1, OUTLET_GATE_CLOSED);

  upper.Close();
  lower.Close();

  close(servoFD);
  close(dispenserFD);
  close(arduinoFD);

  return 0;
}
