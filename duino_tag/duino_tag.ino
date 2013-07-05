#include <IRremote.h>
/* Arduino based example of lazertag protocol.
 *     Copyright 2013 Kevin Osborn, Nick Hollander
 *      Free for reuse,just give us a credit (Creative Commons Attribution)
 *         Hook an IR LED up to Digital pin 3 (PWM) for Arduino Uno etc. (look in IRremote library for other platforms.
 *         Uses Ken Shirrif's IRremote library found at: https://github.com/shirriff/Arduino-IRremote
 */
IRsend irsend;
void setup() {
  Serial.begin(115200);

}
/* lazertag protocol, originally reverse engineered by  Aaron Nabil and documented at 
 * http://web.archive.org/web/20090304155723/http://lasertagparts.com/ltto.htm
 *
 * 3ms On, 6ms Off, 3ms (preamble) followed by 7 bits spaced apart by 2ms. One is represented by 2ms, zero is 1ms
 * In regular lazertag mode, the player and team bits are zero, but the last two bits signify how powerful the blast is.
 * for this example, we show a shot that does one damage and one that does 4 (max)
 */

unsigned int shootOne[] = {3000,6000,3000,2000,1000,2000,1000,2000,1000,2000,1000,2000,1000,2000,1000,2000,1000};
unsigned int shootFour[] ={3000,6000,3000,2000,1000,2000,1000,2000,1000,2000,1000,2000,1000,2000,2000,2000,2000};

void loop() {
  if (Serial.available() !=0)
  {
    char c = Serial.read();
    if (c == '1')
    {
      // the one confusing thing with the irsend.sendRaw command is that the last argument is labled hz, but it's really 
      //  kiloherz. The lazertag team ops uses the common 38kHz frequency.
      irsend.sendRaw(shootOne,sizeof(shootOne)/sizeof(int),38);
      Serial.println("shootOne");
    }
    if (c == '4')
    {
      irsend.sendRaw(shootFour,sizeof(shootFour)/sizeof(int),38);
      Serial.println("shootFour");
    }    
  delay(100);
  }
}
