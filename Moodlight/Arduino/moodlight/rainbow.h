#ifndef ___RAINBOWH___
#define ___RAINBOWH___
#include "displayMode.h"

class rainbow : public displayMode{
  public:
    rainbow(void);
    void setup(void);
    void draw(void);
  private:
    uint16_t cycle=0;
    uint32_t Wheel(byte WheelPos);
    };
rainbow::rainbow(){}
void rainbow::setup(){}
void rainbow::draw(void){
  if (checkTime()){
    if (cycle >= 256*5)
      cycle = 0;
    else 
      cycle++;
    for(int i=0; i< strip.numPixels(); i++) {
      setPixelColor2(i, Wheel(((i * 256 / strip.numPixels()) + cycle) & 255));
      if (checkButton())
        return;
      }
      show2();
  }
}
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t rainbow::Wheel(byte WheelPos) {
    if(WheelPos < 85) {
     return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    } else if(WheelPos < 170) {
     WheelPos -= 85;
     return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else {
     WheelPos -= 170;
     return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}
#endif

