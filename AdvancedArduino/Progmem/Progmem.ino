#include <avr/pgmspace.h>
const char string1[] PROGMEM ={"The Rain in Spain"};
const char string2[] PROGMEM ={"falls mainly in the plain"};
const char* const myStrings[] PROGMEM ={string1,string2};
char buffer[256];
void setup() {
  Serial.begin(115200);
  for (int i =0; i < 2; i++){
    strcpy_P(buffer, (PGM_P)pgm_read_word(&myStrings[i]));
    Serial.println(buffer);
  }
  strcpy_P(buffer, (PGM_P)&string1);
  Serial.println(buffer);
}

void loop() {
  // put your main code here, to run repeatedly:

}
