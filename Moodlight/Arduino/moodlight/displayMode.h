#ifndef __DISPLAYMODEH__
#define __DISPLAYMODEH__
#include <Arduino.h>
#include "Adafruit_NeoPixel.h"
extern Adafruit_NeoPixel strip,strip1;
extern void setPixelColor2(uint16_t pixel, uint32_t color);
extern void show2();
extern boolean checkButton();
class displayMode{
  public:
    displayMode(void){};
    virtual void setup(void);
    virtual void draw(void);
    void setDelay(uint32_t Delay){
      this->wait_time = Delay;
    }
    boolean checkTime(){
       uint32_t currentTime = millis();
       if (currentTime - last_draw > wait_time){
        last_draw = currentTime;
        return true;
       }
       else
        return false;
    }
  private:
    uint32_t wait_time;
    uint32_t last_draw;
  
};
#endif

