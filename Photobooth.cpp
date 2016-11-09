/* Copyright (c) 2016, FlySorter LLC *
 *                                   *
 *                                   */ 

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

/* Here's what we do:
   - Set up GPIOs.
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


int main()
{
  const char * servoCtrl = "/dev/ttyACM0";
  const char * dispenser = "/dev/ttyACM0";

  // Open the Maestro's virtual COM port.
  int fd = open(servoCtrl, O_RDWR | O_NOCTTY);
  if (fd == -1)
  {
    perror(servoCtrl);
    return 1;
  } else {
    printf("FD opened: %d.\n", fd);
  }
 
  struct termios options;
  tcgetattr(fd, &options);
  options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
  options.c_oflag &= ~(ONLCR | OCRNL);
  options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tcsetattr(fd, TCSANOW, &options);

  printf("Options set.\n");
 
  int position = maestroGetPosition(fd, 0);
  printf("Current position is %d.\n", position);
 
  int target = (position < 5000) ? 6000 : 4000;
  printf("Setting target to %d (%d us).\n", target, target/4);
  maestroSetTarget(fd, 0, target);
 
  close(fd);
  return 0;
}
