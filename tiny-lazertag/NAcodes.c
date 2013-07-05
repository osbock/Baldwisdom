#include <avr/io.h>             // this contains all the IO port definitions
#include <avr/pgmspace.h>       // definitions or keeping constants in program memory
#include "main.h"

// table of POWER codes

#define freq_to_timerval(x) ((F_CPU / x - 1 )/ 2)

  
// Code 000 --Lazertag shot
const struct powercode shotCode PROGMEM = {
  freq_to_timerval(38400), // 37.478.4 KHz  
  {{300,600},
   {300, 200},
   {100 , 200},
   {100 , 200},
   {100 , 200},
   {100 , 200},
   {100 , 200},
   {200 , 200},
   {200 , 200},
   {0,0}}
  };   
  



