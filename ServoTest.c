// NOTE: The Maestro's serial mode must be set to "USB Dual Port".
// NOTE: You must change the 'const char * device' line below.

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

// Gets the position of a Maestro channel.
// See the "Serial Servo Commands" section of the user's guide.
int maestroGetPosition(int fd, unsigned char channel)
{
  int n;
  unsigned char command[] = {0x90, channel};
  n = write(fd, command, sizeof(command));
  if(n == -1)
  {
    perror("error writing");
    return -1;
  }
  printf("mgp: wrote command (%d bytes) %02X %02X.\n", n,
    command[0], command[1]);

  usleep(50000);
 
  unsigned char response[2];
  n = read(fd, response, 2);
  if( n != 2)
  {
    printf("n = %d\n", n);
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
 
int main(int argc, char **argv)
{
  // Open the Maestro's virtual COM port.
  const char * device = "/dev/ttyACM0";  // Linux
  int fd = open(device, O_RDWR | O_NOCTTY);
  if (fd == -1)
  {
    perror(device);
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

  int servoNum = 0;

  if ( argc > 1 ) {
    servoNum = atoi(argv[1]);
  }

 
  int position = maestroGetPosition(fd, servoNum);
  printf("Current position is %d.\n", position);

  int target = 0;
  if ( argc > 2 ) {
    target = atoi(argv[2]);
  } else {
    target = (position < 5000) ? 8000 : 4000;
  }

  printf("Setting target to %d (%d us).\n", target, target/4);
  maestroSetTarget(fd, servoNum, target);
 
  close(fd);
  return 0;
}
