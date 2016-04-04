#include "displayMode.h"
#define potPin A1
class potcolor : public displayMode{
  public:
    potcolor(void);
    void setup(void);
    void draw(void);
  private:
  uint32_t Wheel(byte wheelPos);
};
potcolor::potcolor(){}
void potcolor::setup(){
  pinMode(potPin,INPUT);
  }
void potcolor::draw(void){
  if (checkTime()){
    for(int i=0; i< strip.numPixels(); i++) {
      int reading = analogRead(potPin);
      // change the map below to get your full scale reading.
      // for some reason, I'm not getting a full range (0-1023)
      byte mappedReading = (byte)map(reading,0,990,0,254);
/*
      Serial.print("reading,mappedreading: ");
      Serial.print(reading);Serial.print(",");
      Serial.println(mappedReading);
      */

      uint32_t wheelcolor = Wheel(mappedReading);
      setPixelColor2(i, wheelcolor);
      if (checkButton())
        return;
      }
      show2();
  }
}
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t potcolor::Wheel(byte WheelPos) {
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

