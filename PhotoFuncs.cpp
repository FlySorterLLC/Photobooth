/* Copyright (c) 2016/2017, FlySorter LLC *
 *                                        *
 *                                        */ 

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include "PhotoFuncs.h"

using namespace std;

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

int sendSerialCmd(int fd, char *msg, const char *reply,
                  int waitTime /* = 500000 */) {
  int n;
  char replyString[100];

  tcflush(fd, TCIOFLUSH); usleep(100000);

  n = write(fd, msg, strlen(msg));
  if ( n != strlen(msg) ) { perror("error writing to dispenser"); return -1; }

  usleep(waitTime);

  n = serialport_read_until(fd, replyString, '\n', 100, 2000);
  if ( n == 0 ) {
    if ( strcmp(replyString, reply) == 0 ) {
      return 0;
    } else {
      printf("Expected '%s', received: '%s'\n", reply, replyString);
      tcflush(fd, TCIOFLUSH);
      return 1;
    }
  } else {
    perror("error reading from device"); return -1;
  }

}

int initDispenser(int fd) {
  return sendSerialCmd(fd, "I", "ok\n", 1500000);
}

int dispenseFly(int fd) {
  return sendSerialCmd(fd, "F", "ok\n");
}

int enableLights(int fd) {
  return sendSerialCmd(fd, "A\n", "A\n");
}

int disableLights(int fd) {
  return sendSerialCmd(fd, "O\n", "O\n");
}

int pumpOn(int fd) {
  return sendSerialCmd(fd, "P\n", "P\n");
}

int pumpOff(int fd) {
  return sendSerialCmd(fd, "p\n", "p\n");
}

int stepVanes(int fd) {
  return sendSerialCmd(fd, "S\n", "S\n");
}

int stepperOff(int fd) {
  return sendSerialCmd(fd, "s\n", "s\n");
}

