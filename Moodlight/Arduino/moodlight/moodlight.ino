#include "rainbow.h"
#include "potcolor.h"
#include "breathe.h"
#include "cm1.h"
#include "Adafruit_NeoPixel.h"
// pin defines. Note potcolor.h defines the Potentiometer pin
//left LED strip/matrix
#define LEFT 4
//Right LED strip/matrix
#define RIGHT 11
//Mode Button
#define BUTTONPIN 3
Adafruit_NeoPixel strip = Adafruit_NeoPixel(128, LEFT, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(128,RIGHT, NEO_GRB + NEO_KHZ800);

// mode definitions (also include the appropriate header file above.
rainbow myRainbow;
potcolor myPotcolor;
breathe myBreathe;
cm1 myCm1;
displayMode* modesList[] = {&myRainbow,&myPotcolor,&myBreathe,&myCm1};
#define NUMMODES sizeof(modesList)/sizeof(displayMode*)
int mode = NUMMODES; // if mode is one greater than the actual number of modes, turn off display. start off off.


void setup() {
  pinMode(BUTTONPIN,INPUT_PULLUP);
  strip.begin();
  strip1.begin();
  strip.setBrightness(50);
  strip1.setBrightness(50);
  show2();
  Serial.begin(115200);
  for (int i =0; i < NUMMODES; i++)
  {
    Serial.print("Setting up mode: ");Serial.println(i);
    modesList[i]->setup();
    modesList[i]->setDelay(20);
  }
  myBreathe.setDelay(0);
}
void show2(){
  strip.show();
  strip1.show();
}
void setPixelColor2(uint16_t pixel, uint32_t color){
  strip.setPixelColor(pixel,color);
  strip1.setPixelColor(pixel,color); 
}
#define BOUNCEDELAY 150
long lastDebounceTime = 0L;
int buttonState = HIGH;
int lastButtonState = HIGH;
boolean checkButton(){
  boolean retval = false;
  if ((millis() -lastDebounceTime) > BOUNCEDELAY){
    int reading = digitalRead(BUTTONPIN);
    if (reading != lastButtonState){
      //reset the debounce timer
      lastDebounceTime = millis();
      if (reading == LOW){
        // advance mode
        mode = mode+1;
        if (mode > NUMMODES)
          mode = 0;
        retval = true; 
        Serial.println(mode);
      }
    }
    lastButtonState = reading;
  }
  return retval;
}

void loop() {
  checkButton();
  if (mode < NUMMODES){
    modesList[mode]->draw();
  }
  else
  {
    //blank
    for (int i = 0; i < strip.numPixels(); i++)
      setPixelColor2(i,0);
      show2();
  }

}
