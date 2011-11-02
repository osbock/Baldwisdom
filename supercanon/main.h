struct codeElement {
  uint16_t onTime;   // duration of "On" time
  uint16_t offTime;  // duration of "Off" time
};

struct powercode {
  uint8_t timer_val; // not the actual frequency, but the timer value to generate the frequency
  struct codeElement codes[];  // flexible number of on/off codes
};
void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code );
void flashslowLEDx( uint8_t num_blinks );
void quickflashLEDx( uint8_t x );
void tvbgone_sleep( void );
void delay_ten_us(uint16_t us);
void quickflashLED( void );
