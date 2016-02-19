#define BUTTON 2
#define LED 8
static void buttonHandler();
void setup() {
  pinMode(BUTTON,INPUT_PULLUP);
  pinMode(LED,OUTPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON),buttonHandler, LOW);
  Serial.begin(115200);
  interrupts();

}
volatile bool Button_state = false;
volatile unsigned long last_interrupt_time = 0UL;
#define DEBOUNCETIME 1
void loop() {
    digitalWrite(LED,Button_state);
  }
void buttonHandler(){
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > DEBOUNCETIME){
    Button_state = !Button_state;
  }
  last_interrupt_time = interrupt_time;
}

