#include "displayMode.h"
#define potPin A1
// "Connection Machine" style display. Random blinky Red lights
// speed is set with Potentiometer
class cm1 : public displayMode{
  public:
    cm1(void);
    void setup(void);
    void draw(void);
  private:
  uint32_t Wheel(byte wheelPos);
};
cm1::cm1(){}
void cm1::setup(){
  pinMode(potPin,INPUT);
  randomSeed(A5); //make sure this is not hooked up
  }
void cm1::draw(void){
  if (checkTime()){
    for(int i=0; i< strip.numPixels(); i++) {
      int reading = analogRead(potPin);
      setDelay(reading);
      if(!random(0,3))
        setPixelColor2(i, strip.Color(180,0,0));
      else
        setPixelColor2(i,0);
      if (checkButton())
        return;
      }
      show2();
  }
}


