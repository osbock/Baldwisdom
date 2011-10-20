#include <avr/io.h>             // this contains all the IO port definitions
#include <avr/pgmspace.h>       // definitions or keeping constants in program memory
#include "main.h"

// table of POWER codes

#define freq_to_timerval(x) ((F_CPU / x - 1 )/ 2)

  
// Code 000 -- Nikon remote trigger
const struct powercode NikonCode PROGMEM = {
  freq_to_timerval(37470), // 37.47 KHz  
  {{200, 2800},
   {50, 150},
   {50 , 350},
   {50, 6300},
   {200, 2800},
   {50, 150},
   {50 , 350},
   {50, 0}}
  };   
  



