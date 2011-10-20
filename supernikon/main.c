/*
Super Nikon Remote firmware by Kevin Osborn
Updated for v1.2 TV-B-Gone hardware
based on 
TV-B-Gone
Firmware
for use with ATtiny85
Mitch Altman + Limor Fried
7-Oct-07

Distributed under Creative Commons 2.5 -- Attib & Share Alike
*/

#include <avr/io.h>             // this contains all the IO port definitions
#include <avr/interrupt.h>      // definitions for interrupts
#include <avr/sleep.h>          // definitions for power-down modes
#include <avr/pgmspace.h>       // definitions or keeping constants in program memory
#include "main.h"
#include <avr/wdt.h>

#define LED PB2         //  visible LED
#define IRLED PB0      //  IR LED

/*
This project transmits a bunch of TV POWER codes, one right after the other, 
with a pause in between each.  (To have a visible indication that it is 
transmitting, it also pulses a visible LED once each time a POWER code is 
transmitted.)  That is all TV-B-Gone does.  The tricky part of TV-B-Gone 
was collecting all of the POWER codes, and getting rid of the duplicates and 
near-duplicates (because if there is a duplicate, then one POWER code will 
turn a TV off, and the duplicate will turn it on again (which we certainly 
do not want).  I have compiled the top-40 most popular codes with the 
duplicates eliminated, both for North America (which is the same as Asia, as 
far as POWER codes are concerned -- even though much of Asia USES PAL video) 
and for Europe (which works for Australia, New Zealand, the Middle East, and 
other parts of the world that use PAL video).

Before creating a TV-B-Gone Kit, I originally started this project by hacking 
the MiniPOV kit.  This presents a limitation, based on the size of
the Atmel ATtiny2313 internal flash memory, which is 2KB.  With 2KB we can only 
fit about 7 POWER codes into the firmware's database of POWER codes.  40 codes
requires 8KB of flash memory, which is why we chose the ATtiny85 for the 
TV-B-Gone Kit.

This version of the firmware has the most popular 40 POWER codes for North America.
*/


/*
This project is a good example of how to use the AVR chip timers.
*/


/*
The hardware for this project is very simple:
     ATtiny85 has 8 pins:
       pin 1   connects to programming circuitry
       pin 2   one pin of ceramic resonator
       pin 3   one pin of ceramic resonator
       pin 4   ground
       pin 5   PB0 - visible LED, and also connects to programming circuitry
       pin 6   OC1A - IR emitter, through a PN2222A driver (with 47 ohm base resistor), and also connects to programming circuitry
       pin 7   push-button switch, and also connects to serial port programming circuitry
       pin 8   +3v
    See the schematic for more details.

    This firmware requires using an 8.0MHz ceramic resonator 
       (since the internal oscillator may not be accurate enough).

    IMPORTANT:  to use the ceramic resonator, you must perform the following:
                    make burn-fuse_cr
*/


/*
The C compiler creates code that will transfer all constants into RAM when the microcontroller
resets.  Since this firmware has a table (powerCodes) that is too large to transfer into RAM,
the C compiler needs to be told to keep it in program memory space.  This is accomplished by
the macro PROGMEM (this is used in the definition for powerCodes).  Since the
C compiler assumes that constants are in RAM, rather than in program memory, when accessing
powerCodes, we need to use the pgm_read_word() and pgm_read_byte macros, and we need
to use powerCodes as an address.  This is done with PGM_P, defined below.  
For example, when we start a new powerCode, we first point to it with the following statement:
    PGM_P thecode_p = pgm_read_word(powerCodes+i);
The next read from the powerCode is a byte that indicates the carrier frequency, read as follows:
    uint8_t freq = pgm_read_byte(thecode_p);
Subsequent reads from the powerCode are onTime/offTime pairs, which are words, read as follows:
    ontime = pgm_read_word(thecode_p+(offset_into_table);
    offtime = pgm_read_word(thecode_p+(offset_into_table);
*/

#define NOP __asm__ __volatile__ ("nop")
// Tweak this if neccessary to change timing
#define DELAY_CNT 11





// This function quickly pulses the visible LED (connected to PB0, pin 5) 4 times
void quickflashLED4x( void ) {
  quickflashLED();
  delay_ten_us(15000);        // 150 millisec delay
  quickflashLED();
  delay_ten_us(15000);        // 150 millisec delay
  quickflashLED();
  delay_ten_us(15000);        // 150 millisec delay
  quickflashLED();
}



/* This function is the 'workhorse' of transmitting IR codes.
   Given the on and off times, it turns on the PWM output on and off
   to generate one 'pair' from a long code. Each code has ~50 pairs! */
void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code )
{
  // start Timer0 outputting the carrier frequency to IR emitters on and OC0A 
  // (PB0, pin 5)
  TCNT0 = 0; // reset the timers so they are aligned
  TIFR = 0;  // clean out the timer flags

  if(PWM_code) {
    // 99% of codes are PWM codes, they are pulses of a carrier frequecy
    // Usually the carrier is around 38KHz, and we generate that with PWM
    // timer 0
    TCCR0A =_BV(COM0A0) | _BV(WGM01);          // set up timer 0
    TCCR0B = _BV(CS00);
  } else {
    // However some codes dont use PWM in which case we just turn the IR
    // LED on for the period of time.
    PORTB &= ~_BV(IRLED);
  }

  // Now we wait, allowing the PWM hardware to pulse out the carrier 
  // frequency for the specified 'on' time
  delay_ten_us(ontime);
  
  // Now we have to turn it off so disable the PWM output
  TCCR0A = 0;
  TCCR0B = 0;
  // And make sure that the IR LED is off too (since the PWM may have 
  // been stopped while the LED is on!)
  PORTB |= _BV(IRLED);           // turn off IR LED

  // Now we wait for the specified 'off' time
  delay_ten_us(offtime);
}
//extern const struct powercode powerCodes[] PROGMEM;
extern const struct powercode NikonCode PROGMEM;

extern uint8_t num_codes;

int main(void) {
  uint8_t i, j;
  uint16_t ontime, offtime;

  
  TCCR1 = 0;		   // Turn off PWM/freq gen, should be off already
  TCCR0A = 0;
  TCCR0B = 0;

  i = MCUSR;                     // Save reset reason
  MCUSR = 0;                     // clear watchdog flag
  WDTCR = _BV(WDCE) | _BV(WDE);  // enable WDT disable

  WDTCR = 0;                     // disable WDT while we setup

  DDRB = _BV(LED) | _BV(IRLED);   // set the visible and IR LED pins to outputs
  PORTB = _BV(LED) |              //  visible LED is off when pin is high
    _BV(IRLED);            // IR LED is off when pin is high



  // check the reset flags
  if (i & _BV(BORF)) {    // Brownout
    // Flash out an error and go to sleep
    flashslowLEDx(2);	
    tvbgone_sleep();  
  }

  delay_ten_us(5000);            // Let everything settle for a bit
  // Starting execution loop
  delay_ten_us(25000);
  
  // turn on watchdog timer immediately, this protects against
  // a 'stuck' system by resetting it
  wdt_enable(WDTO_8S); // 1 second long timeout

  for (i=0; i<1; i++) {   // repeat the code twice
      //To keep Watchdog from resetting in middle of code.
    wdt_reset();
    quickflashLED(); // visible indication that a code is being output
    uint8_t freq = pgm_read_byte(&NikonCode);
    // set OCR for Timer1 and Timer0 to output this POWER code's carrier frequency
    OCR0A = OCR0B = freq; 
    
    // transmit all codeElements for this POWER code (a codeElement is an onTime and an offTime)
    // transmitting onTime means pulsing the IR emitters at the carrier frequency for the length of time specified in onTime
    // transmitting offTime means no output from the IR emitters for the length of time specified in offTime
    j = 0;  // index into codeElements of this POWER code
    do {
      // read the onTime and offTime from the program memory
      ontime = pgm_read_word(&NikonCode+(j*4)+1);
      offtime = pgm_read_word(&NikonCode+(j*4)+3);

      xmitCodeElement(ontime, offtime, 1);  // transmit this codeElement (ontime and offtime)
      j++;
    } while ( offtime != 0 );  // offTime = 0 signifies last codeElement for a POWER code

    PORTB |= _BV(IRLED);           // turn off IR LED

    // delay 250 milliseconds before transmitting next POWER code
    delay_ten_us(25000);
  }
    // We are done, no need for a watchdog timer anymore
  wdt_disable();

  // flash the visible LED on PB0  4 times to indicate that we're done
  delay_ten_us(65500); // wait maxtime 
  quickflashLED4x();

  // Shut down everything and put the CPU to sleep
  TCCR1 = 0;                      // turn off frequency generator (should be off already)
  TCCR0B = 0;
  PORTB |= _BV(LED); // turn on the button pullup, turn off visible LED
  PORTB |= _BV(IRLED);          // turn off IR LED
  delay_ten_us(1000);             // wait 10 millisec second

  MCUCR = _BV(SM1) |  _BV(SE);    // power down mode,  SE=1 (bit 5) -- enables Sleep Modes
  sleep_cpu();                    // put CPU inteo Power Down Sleep Mode
}

/****************************** SLEEP FUNCTIONS ********/

void tvbgone_sleep( void )
{
  // Shut down everything and put the CPU to sleep
  TCCR0A = 0;           // turn off frequency generator (should be off already)
  TCCR0B = 0;           // turn off frequency generator (should be off already)
  PORTB |= _BV(LED) |       // turn off visible LED
           _BV(IRLED);     // turn off IR LED

  wdt_disable();           // turn off the watchdog (since we want to sleep
  delay_ten_us(1000);      // wait 10 millisec

  MCUCR = _BV(SM1) |  _BV(SE);    // power down mode,  SE enables Sleep Modes
  sleep_cpu();                    // put CPU into Power Down Sleep Mode
}


/****************************** LED AND DELAY FUNCTIONS ********/


// This function delays the specified number of 10 microseconds
// it is 'hardcoded' and is calibrated by adjusting DELAY_CNT 
// in main.h Unless you are changing the crystal from 8mhz, dont
// mess with this.
void delay_ten_us(uint16_t us) {
  uint8_t timer;
  while (us != 0) {
    // for 8MHz we want to delay 80 cycles per 10 microseconds
    // this code is tweaked to give about that amount.
    for (timer=0; timer <= DELAY_CNT; timer++) {
      NOP;
      NOP;
    }
    NOP;
    us--;
  }
}


// This function quickly pulses the visible LED (connected to PB0, pin 5)
// This will indicate to the user that a code is being transmitted
void quickflashLED( void ) {
  PORTB &= ~_BV(LED);   // turn on visible LED at PB0 by pulling pin to ground
  delay_ten_us(3000);   // 30 millisec delay
  PORTB |= _BV(LED);    // turn off visible LED at PB0 by pulling pin to +3V
}

// This function just flashes the visible LED a couple times, used to
// tell the user what region is selected
void quickflashLEDx( uint8_t x ) {
  quickflashLED();
  while(--x) {
  	wdt_reset();
	delay_ten_us(15000);     // 150 millisec delay between flahes
	quickflashLED();
  }
  wdt_reset();                // kick the dog
}

// This is like the above but way slower, used for when the tvbgone
// crashes and wants to warn the user
void flashslowLEDx( uint8_t num_blinks )
{
  uint8_t i;
  for(i=0;i<num_blinks;i++)
    {
      // turn on visible LED at PB0 by pulling pin to ground
      PORTB &= ~_BV(LED);    
      delay_ten_us(50000);         // 500 millisec delay
      wdt_reset();                 // kick the dog
      // turn off visible LED at PB0 by pulling pin to +3V
      PORTB |= _BV(LED);          
      delay_ten_us(50000);	   // 500 millisec delay
      wdt_reset();                 // kick the dog
    }
}
