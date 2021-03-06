// NOTE: You must change the 'const char * device' line below.

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

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
        printf("serialport_read_until: i=%d, n=%d b='%c' (%02X)\n",i,n,b[0], b[0]); // debug
        buf[i] = b[0]; 
        i++;
    } while( b[0] != until && i < buf_max && timeout>0 );

    buf[i] = 0;  // null terminate the string
    return 0;
}

int main()
{
  // Open the Dispenser's virtual COM port.
  const char * device = "/dev/ttyACM2";
  int fd = open(device, O_RDWR | O_NONBLOCK );
  if (fd == -1)
  {
    perror(device);
    return 1;
  } else {
    printf("FD opened: %d.\n", fd);
  }
 
  struct termios options;
  if ( tcgetattr(fd, &options) < 0 ) {
    printf("Error reading termios option.\n");
    close(fd);
    return 1;
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
    return -1;
  }

  printf("Options set.\n");

  usleep(500000);

  char versionString[100];
  const char *verCmd = "V";

  int n = write(fd, verCmd, strlen(verCmd));
  if ( n != strlen(verCmd) )
  {
    perror("error writing");
    return -1;
  }
  printf("Wrote command: %s (%d bytes)\n", verCmd, n);

  usleep(500000);
  
  printf("Reading...\n");
  n = serialport_read_until(fd, versionString, '\n', 100, 2000);
  printf("serialport_read_until returned %d.\n", n);
  if ( n != -1 ) {
    printf("Read: %s\n(%d chars)\n", versionString, strlen(versionString));
    printf("Byte %d: %02X\n", strlen(versionString)-1, versionString[strlen(versionString)-1]);
  }

  tcflush(fd, TCIOFLUSH);

  n = serialport_read_until(fd, versionString, '\n', 100, 2000);
  printf("serialport_read_until returned %d.\n", n);
  if ( n != -1 ) {
    printf("Byte 0: %02X\n", versionString[0]);
  }

  n = serialport_read_until(fd, versionString, '\n', 100, 2000);
  printf("serialport_read_until returned %d.\n", n);
  if ( n != -1 ) {
    printf("Byte 0: %02X\n", versionString[0]);
  }

/*  int br = -1;
  br = read(fd, versionString, 1);
  printf("Read %d bytes.\n", br);
  if ( br > 0 ) {
    printf("Read: %s.\n", versionString);
  } */

  close(fd);
  return 0;
}
