#include <avr/io.h>             // this contains all the IO port definitions
#include <avr/pgmspace.h>       // definitions or keeping constants in program memory
#include "main.h"

// table of POWER codes

#define freq_to_timerval(x) ((F_CPU / x - 1 )/ 2)

  
// Code 000 -- Canon remote trigger
const struct powercode CanonCode PROGMEM = {
  freq_to_timerval(32600), // 32600 KHz  
    {{56, 733},
     {56, 0}}
  };   
  



