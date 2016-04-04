#ifndef ___BREATHH___
#define ___BREATHH___
#include "displayMode.h"

class breathe : public displayMode{
  public:
    breathe(void);
    void setup(void);
    void draw(void);
  private:
    uint16_t cycle=0;
    uint16_t increment =1;
    };
breathe::breathe(){}
void breathe::setup(){}
void breathe::draw(void){
  if (checkTime()){
    cycle += increment;
    if (cycle >= 180)
      increment = -1;
    if (cycle <=0)
      increment =1;
    for(int i=0; i< strip.numPixels(); i++) {
      setPixelColor2(i,strip.Color(cycle,0,0));
      if (checkButton())
        return;
      }
      show2();
  }
}

#endif

