#include <ISD.h>
// delay between motion activations
#define ACTDELAY 10000L
// motion sensor
#define MOTION A4
#define ARMButton A5
// powerswitch tail 
#define powerSwitch 7
// LED
#define LEDPin 13


ISD isd = ISD();

void setup() {
  Serial.begin(115200);
  // motion sensor
  pinMode(MOTION, INPUT);
  // open collector requires pullup
  digitalWrite(MOTION,HIGH);
    // powerswitch
  pinMode(powerSwitch,OUTPUT);
  digitalWrite(powerSwitch,LOW);
  pinMode(ARMButton, INPUT_PULLUP);
  pinMode(LEDPin,OUTPUT);
//Serial.println("enter a message number to play or record or r/p");
}
// globals
int msg =0;
boolean motion = false;
boolean soundonmotion = true;
boolean powerswitchonmotion = true;
int lastReading = HIGH;
boolean ARMState = false;
long lastActivation = 0L;
void loop() {
  // check arming button
  if (digitalRead(ARMButton) == LOW){
    ARMState = ARMState?false:true;
    // cheap debounce
    delay(500);
  
  if (ARMState){
    digitalWrite(13,HIGH);
    soundonmotion = true;
    motion = true;
    powerswitchonmotion = true;
    Serial.println("Armed!");
  }
  else{
    digitalWrite(13,LOW);
    motion = soundonmotion = powerswitchonmotion = false;
    Serial.println("Unarmed");
  }
  }
    //check motion sensor
  long currentTime = millis();
  if (motion){
     int currentReading = digitalRead(MOTION);
    if ((currentReading != lastReading) && (currentReading == LOW) && ((currentTime - lastActivation) > ACTDELAY)){
      lastActivation = currentTime;
         Serial.println("activating motion");

    
    // if enabled turn fan on first
      if (powerswitchonmotion){
        digitalWrite(powerSwitch,HIGH);
        delay(1000);
      }
      if (soundonmotion)
        isd.play(4);
      if (powerswitchonmotion){
        delay(5000);
        digitalWrite(powerSwitch,LOW);
      }
        
      delay(1000);
    }
    lastReading = currentReading;
  }
    
  if (Serial.available() != 0)
  {
    char c = Serial.read();
    if (c >= '0' && c <='8')
    {
      msg = (int)(c- '0');

      Serial.print("msg selected: ");
      Serial.println(msg);
    }
    else if (c == 'r')
    {
      isd.record(msg);
    }
     else if (c == 'p')
       isd.play(msg);
    else if (c == 'm'){
      //toggle motion activation
      motion = motion?false:true;
      Serial.print("motion is ");
      Serial.println(motion);
    }
    else if (c == 'f'){
        powerswitchonmotion = powerswitchonmotion?false:true;
        Serial.print("powerswitch is ");
        Serial.println(powerswitchonmotion);
      }
    else if( c == 's'){
      soundonmotion = soundonmotion?false:true;
      Serial.print("sound is ");
      Serial.println(soundonmotion);
    }
      
  }
}


