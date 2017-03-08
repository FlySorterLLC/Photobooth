/*
 */

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#define BLINK_PIN  0

int main (void)
{

  wiringPiSetup();
  pinMode(BLINK_PIN, OUTPUT);

  for (;;)
  {
    printf ("on\n");
    digitalWrite (BLINK_PIN, HIGH) ;	// On
    delay (2000) ;		// mS
    printf ("off\n");
    digitalWrite (BLINK_PIN, LOW) ;	// Off
    delay (2000) ;
  }
  return 0 ;

}
