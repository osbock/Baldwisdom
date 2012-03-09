//#include <SoftwareSerial.h>
#include "stk500.h"
#include <SD.h>
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

//sd card stuff
File root;
File recordFile;
const int chipSelect = 10;
//Adafruit LCD shield
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define OPTIBOOT_MINVER 4
#define OPTIBOOT_MAJVER 4
/* Read hex file from sd/micro-sd card and program
 *  another arduino via ttl serial.
 * borrows heavily from avrdude source code (GPL license)
 *   Copyright (C) 2002-2004 Brian S. Dean <bsd@bsdhome.com>
 *   Copyright (C) 2008 Joerg Wunsch
 * Recording code borrowed from optiboot
 *  Created 12/26/2011 Kevin Osborn
 */
 

//Arduino UNO
#define BOOT_BAUD 115200 
// Adruino Duemilanove with 328
//#define BOOT_BAUD 57600
#define DEBUG_BAUD 19200
// different pins will be needed for I2SD, as 2/3 are leds
#define txPin 4
#define rxPin 6
#define rstPin 5

//indicator LEDs on I2SD
#define LED1 13
#define LED2 3
//SoftwareSerial sSerial= SoftwareSerial(rxPin,txPin);
// set up variables using the SD utility library functions:
File myFile;
avrmem mybuf;
unsigned char mempage[128];
#define buff mybuf.buf

//chipselect for the wyolum i2sd is 10

// STANDALONE_DEBUG sends error messages out the main 
// serial port. Not useful after you are actually trying to slave
// another arduino
//#define STANDALONE_DEBUG
/*#ifdef STANDALONE_DEBUG
#define DEBUGPLN Serial.println
#define DEBUGP Serial.print
#else
#define DEBUGPLN sSerial.println
#define DEBUGP sSerial.print
#endif
*/

void setup() {
  lcd.begin(16,2);
  mybuf.buf = &mempage[0];
  //sSerial.begin(DEBUG_BAUD);
  // and the regular serial port for error messages, etc.
  Serial.begin(BOOT_BAUD);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(rstPin,OUTPUT);
  pinMode(chipSelect,OUTPUT);
  pinMode(LED1,OUTPUT);
  digitalWrite(rstPin,HIGH);
    // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    lcd.print("Card failed");
    // don't do anything more:
    return;
  }
  lcd.print("card init");
  delay(1000);

  root = SD.open("/");
  lcd.setBacklight(VIOLET);
  lcd.clear();
  lcd.print("Bootdrive");
}

File currentEntry;
void loop()
{
    uint8_t buttons = lcd.readButtons();

  if (buttons) {
    if (buttons & BUTTON_DOWN) {
      lcd.setBacklight(VIOLET);
      lcd.clear();
      lcd.setCursor(0,0);
      currentEntry = root.openNextFile();
      if (! currentEntry) {
       // no more files (rewind?)
       //root.close();
       //root = SD.open("/");
       root.rewindDirectory();
       currentEntry = root.openNextFile();
      }
      lcd.print(currentEntry.name());
      currentEntry.close(); // will get reopened by name when selected
      
    }else if (buttons & BUTTON_SELECT){
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print("programming");
      lcd.setCursor(0,1);
      lcd.print(currentEntry.name());
      programArduino(currentEntry.name());
      lcd.setBacklight(GREEN);
      lcd.clear();
      lcd.print("done");
    }else if (buttons & BUTTON_LEFT){
      lcd.clear();
      lcd.setBacklight(TEAL);
      lcd.setCursor(0,0);
      lcd.print("recording");
      record();
      lcd.setBacklight(GREEN);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("done");
    }
    delay(500); // crude debounce
    }
  }

// Line Buffer is set up in global SRAM
#define LINELENGTH 50
unsigned char linebuffer[LINELENGTH];
unsigned char linemembuffer[16];
int readPage(File input, avrmem *buf, boolean binaryFlag)
{
  int len;
  int address;
  int total_len =0;
  
  if (binaryFlag){ // reading binary file, read a whole page at once
    len = input.read(buf->buf,128);
    total_len=len;
    //buf->pageaddress = buf->pageaddress + 128; // if loading a binary file, just add a page to the last address
  }else
  // grab 128 bytes or less (one page)
  for (int i=0 ; i < 8; i++){
    len = readIntelHexLine(input, &address, &linemembuffer[0]);
    if (len <= 0)
      break;
    else
      total_len=total_len+len;
    if (i==0)// first record determines the page address
      buf->pageaddress = address;
    memcpy((buf->buf)+(total_len-len), linemembuffer, len);
  }
  buf->size = total_len;
  return total_len;
  
}
// read one line of intel hex from file. Return the number of databytes
// Since the arduino code is always sequential, ignore the address for now.
// If you want to burn bootloaders, etc. we'll have to modify to 
// return the address.

// INTEL HEX FORMAT:
// :<8-bit record size><16bit address><8bit record type><data...><8bit checksum>
int readIntelHexLine(File input, int *address, unsigned char *buf){
  unsigned char c;
  int i=0;
  while (true){
    if (input.available()){
      c = input.read();
      // this should handle unix or ms-dos line endings.
      // break out when you reach either, then check
      // for lf in stream to discard
      if ((c == 0x0d)|| (c == 0x0a))
        break;
      else
        linebuffer[i++] =c;
    }
    else return -1; //end of file
  }
  linebuffer[i]= 0; // terminate the string
  //peek at the next byte and discard if line ending char.
  if (input.peek() == 0xa)
    input.read();
  int len = hex2byte(&linebuffer[1]);
  *address = (hex2byte(&linebuffer[3]) <<8) |
               (hex2byte(&linebuffer[5]));
  int j=0;
  for (int i = 9; i < ((len*2)+9); i +=2){
    buf[j] = hex2byte(&linebuffer[i]);
    j++;
  }
  return len;
}
unsigned char hex2byte(unsigned char *code){
  unsigned char result =0;

  if ((code[0] >= '0') && (code[0] <='9')){
    result = ((int)code[0] - '0') << 4;
  }
  else if ((code[0] >='A') && (code[0] <= 'F')) {
    result = ((int)code[0] - 'A'+10) << 4;
  }
  if ((code[1] >= '0') && (code[1] <='9')){
    result |= ((int)code[1] - '0');
  }
  else if ((code[1] >='A') && (code[1] <= 'F'))  
    result |= ((int)code[1] -'A'+10);  
return result;
}


void programArduino(char * filename){

  digitalWrite(rstPin,HIGH);

  unsigned int major=0;
  unsigned int minor=0;
  delay(100);
   toggle_Reset();
   delay(90);
   stk500_getsync();
   stk500_getparm(Parm_STK_SW_MAJOR, &major);
lcd2("sw major: ", major);
  stk500_getparm(Parm_STK_SW_MINOR, &minor);
lcd2("sw Minor: ",minor);

if (SD.exists(filename)){
    myFile = SD.open(filename, FILE_READ);
  }
  else{
   /* DEBUGP(filename);
    DEBUGPLN(" doesn't exist");*/
    return;
  }
    boolean binaryFlag = isBinaryFile(myFile);
    mybuf.pageaddress = 0;
  //enter program mode
  stk500_program_enable();

  while (readPage(myFile,&mybuf,binaryFlag) > 0){
    stk500_loadaddr(mybuf.pageaddress>>1);
    stk500_paged_write(&mybuf, mybuf.size, mybuf.size);
    if (binaryFlag)
      mybuf.pageaddress+= 128;
  }
  
  // could verify programming by reading back pages and comparing but for now, close out
  stk500_disable();
  delay(10);
  toggle_Reset();
  myFile.close();
  
  
}

void toggle_Reset()
{
  digitalWrite(rstPin, LOW);
  delayMicroseconds(2000);
  digitalWrite(rstPin,HIGH);
}
static int stk500_send(byte *buf, unsigned int len)
{

  Serial.write(buf,len);
}
static int stk500_recv(byte * buf, unsigned int len)
{
  int rv;

 
  rv = Serial.readBytes((char *)buf,len);
  if (rv < 0) {
    error(ERRORNOPGMR);
    return -1;
  }
  return 0;
}
int stk500_drain()
{
  while (Serial.available()> 0)
  {  
 //   DEBUGP("draining: ");
 //   DEBUGPLN(Serial.read(),HEX);
  }
  return 1;
}
int stk500_getsync()
{
  byte buf[32], resp[32];

  /*
   * get in sync */
  buf[0] = Cmnd_STK_GET_SYNC;
  buf[1] = Sync_CRC_EOP;
  
  /*
   * First send and drain a few times to get rid of line noise 
   */
  /*
  stk500_send(buf, 2);
  stk500_drain();
  stk500_send(buf, 2);
  stk500_drain();
  */
  stk500_send(buf, 2);
  if (stk500_recv(resp, 1) < 0)
    return -1;
  if (resp[0] != Resp_STK_INSYNC) {
        error1(ERRORPROTOSYNC,resp[0]);
    stk500_drain();
    return -1;
  }

  if (stk500_recv(resp, 1) < 0)
    return -1;
  if (resp[0] != Resp_STK_OK) {
    error1(ERRORNOTOK,resp[0]);
    return -1;
  }
  return 0;
}
static int stk500_getparm(unsigned parm, unsigned * value)
{
  byte buf[16];
  unsigned v;
  int tries = 0;

 retry:
  tries++;
  buf[0] = Cmnd_STK_GET_PARAMETER;
  buf[1] = parm;
  buf[2] = Sync_CRC_EOP;

  stk500_send(buf, 3);

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_NOSYNC) {
    if (tries > 33) {
      error(ERRORNOSYNC);
      return -1;
    }
   if (stk500_getsync() < 0)
      return -1;
      
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC,buf[0]);
    return -2;
  }

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  v = buf[0];

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_FAILED) {
    error1(ERRORPARMFAILED,v);
    return -3;
  }
  else if (buf[0] != Resp_STK_OK) {
    error1(ERRORNOTOK,buf[0]);
    return -3;
  }

  *value = v;

  return 0;
}
/* read signature bytes - arduino version */
static int arduino_read_sig_bytes(AVRMEM * m)
{
  unsigned char buf[32];

  /* Signature byte reads are always 3 bytes. */

  if (m->size < 3) {
    lcd.print("memsize too small");
    return -1;
  }

  buf[0] = Cmnd_STK_READ_SIGN;
  buf[1] = Sync_CRC_EOP;

  stk500_send(buf, 2);

  if (stk500_recv(buf, 5) < 0)
    return -1;
  if (buf[0] == Resp_STK_NOSYNC) {
    error(ERRORNOSYNC);
	return -1;
  } else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC,buf[0]);
	return -2;
  }
  if (buf[4] != Resp_STK_OK) {
    error1(ERRORNOTOK,buf[4]);
    return -3;
  }

  m->buf[0] = buf[1];
  m->buf[1] = buf[2];
  m->buf[2] = buf[3];

  return 3;
}

static int stk500_loadaddr(unsigned int addr)
{
  unsigned char buf[16];
  int tries;

  tries = 0;
 retry:
  tries++;
  buf[0] = Cmnd_STK_LOAD_ADDRESS;
  buf[1] = addr & 0xff;
  buf[2] = (addr >> 8) & 0xff;
  buf[3] = Sync_CRC_EOP;


  stk500_send(buf, 4);

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_NOSYNC) {
    if (tries > 33) {
      error(ERRORNOSYNC);
      return -1;
    }
    if (stk500_getsync() < 0)
      return -1;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC, buf[0]);
    return -1;
  }

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return 0;
  }

  error1(ERRORPROTOSYNC, buf[0]);
  return -1;
}
static int stk500_paged_write(AVRMEM * m, 
                              int page_size, int n_bytes)
{
  // This code from avrdude has the luxury of living on a PC and copying buffers around.
  // not for us...
 // unsigned char buf[page_size + 16];
 unsigned char cmd_buf[16]; //just the header
  int memtype;
 // unsigned int addr;
  int block_size;
  int tries;
  unsigned int n;
  unsigned int i;
  int flash;

  // Fix page size to 128 because that's what arduino expects
  page_size = 128;
  //EEPROM isn't supported
  memtype = 'F';
  flash = 1;


    /* build command block and send data separeately on arduino*/
    
    i = 0;
    cmd_buf[i++] = Cmnd_STK_PROG_PAGE;
    cmd_buf[i++] = (page_size >> 8) & 0xff;
    cmd_buf[i++] = page_size & 0xff;
    cmd_buf[i++] = memtype;
    stk500_send(cmd_buf,4);
    stk500_send(&m->buf[0], page_size);
    cmd_buf[0] = Sync_CRC_EOP;
    stk500_send( cmd_buf, 1);

    if (stk500_recv(cmd_buf, 1) < 0)
      exit(1); // errr need to fix this... 
    if (cmd_buf[0] == Resp_STK_NOSYNC) {
        error(ERRORNOSYNC);
        return -3;
     }
    else if (cmd_buf[0] != Resp_STK_INSYNC) {

     error1(ERRORPROTOSYNC, cmd_buf[0]);
      return -4;
    }
    
    if (stk500_recv(cmd_buf, 1) < 0)
      exit(1);
    if (cmd_buf[0] != Resp_STK_OK) {
    error1(ERRORNOTOK,cmd_buf[0]);

      return -5;
    }
  

  return n_bytes;
}
#ifdef LOADVERIFY //maybe sometime? note code needs to be re-written won't work as is
static int stk500_paged_load(AVRMEM * m, 
                             int page_size, int n_bytes)
{
  unsigned char buf[16];
  int memtype;
  unsigned int addr;
  int a_div;
  int tries;
  unsigned int n;
  int block_size;

  memtype = 'F';


  a_div = 1;

  if (n_bytes > m->size) {
    n_bytes = m->size;
    n = m->size;
  }
  else {
    if ((n_bytes % page_size) != 0) {
      n = n_bytes + page_size - (n_bytes % page_size);
    }
    else {
      n = n_bytes;
    }
  }

  for (addr = 0; addr < n; addr += page_size) {
//    report_progress (addr, n_bytes, NULL);

    if ((addr + page_size > n_bytes)) {
	   block_size = n_bytes % page_size;
	}
	else {
	   block_size = page_size;
	}
  
    tries = 0;
  retry:
    tries++;
    stk500_loadaddr(addr/a_div);
    buf[0] = Cmnd_STK_READ_PAGE;
    buf[1] = (block_size >> 8) & 0xff;
    buf[2] = block_size & 0xff;
    buf[3] = memtype;
    buf[4] = Sync_CRC_EOP;
    stk500_send(buf, 5);

    if (stk500_recv(buf, 1) < 0)
      exit(1);
    if (buf[0] == Resp_STK_NOSYNC) {
      if (tries > 33) {
        error(ERRORNOSYNC);
        return -3;
      }
      if (stk500_getsync() < 0)
	return -1;
      goto retry;
    }
    else if (buf[0] != Resp_STK_INSYNC) {
      error1(ERRORPROTOSYNC, buf[0]);
      return -4;
    }

    if (stk500_recv(&m->buf[addr], block_size) < 0)
      exit(1);

    if (stk500_recv(buf, 1) < 0)
      exit(1);

    if (buf[0] != Resp_STK_OK) {
        error1(ERRORPROTOSYNC, buf[0]);
        return -5;
      }
    }
  

  return n_bytes;
}
#endif

/*
 * issue the 'program enable' command to the AVR device
 */
static int stk500_program_enable()
{
  unsigned char buf[16];
  int tries=0;

 retry:
  
  tries++;

  buf[0] = Cmnd_STK_ENTER_PROGMODE;
  buf[1] = Sync_CRC_EOP;

  stk500_send( buf, 2);
  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_NOSYNC) {
    if (tries > 33) {
      error(ERRORNOSYNC);
      return -1;
    }
    if (stk500_getsync()< 0)
      return -1;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC,buf[0]);
    return -1;
  }

  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return 0;
  }
  else if (buf[0] == Resp_STK_NODEVICE) {
    error(ERRORNODEVICE);
    return -1;
  }

  if(buf[0] == Resp_STK_FAILED)
  {
      error(ERRORNOPROGMODE);
	  return -1;
  }


  error1(ERRORUNKNOWNRESP,buf[0]);

  return -1;
}

static void stk500_disable()
{
  unsigned char buf[16];
  int tries=0;

 retry:
  
  tries++;

  buf[0] = Cmnd_STK_LEAVE_PROGMODE;
  buf[1] = Sync_CRC_EOP;

  stk500_send( buf, 2);
  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_NOSYNC) {
    if (tries > 33) {
      error(ERRORNOSYNC);
      return;
    }
    if (stk500_getsync() < 0)
      return;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    error1(ERRORPROTOSYNC,buf[0]);
    return;
  }

  if (stk500_recv( buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return;
  }
  else if (buf[0] == Resp_STK_NODEVICE) {
    error(ERRORNODEVICE);
    return;
  }

  error1(ERRORUNKNOWNRESP,buf[0]);

  return;
}
//original avrdude error messages get copied to ram and overflow, wo use numeric codes.
void error1(int errno,unsigned char detail){
   error(errno);
   lcd.setCursor(0,1);
   lcd.print(" detail: ");
   lcd.print(detail);
   lcd.setBacklight(VIOLET);
}


void error(int errno){
  lcd.clear();
  lcd.setBacklight(RED);
  lcd.setCursor(0,0);
  lcd.print("error: " );
  lcd.print(errno);
}
void lcd2(char* msg,int detail){
  lcd.clear();
  lcd.print(msg);lcd.print(detail);
}
/*void dumphex(unsigned char *buf,int len)
{
  for (int i = 0; i < len; i++)
  {
    if (i%16 == 0)
      DEBUGPLN();
      DEBUGP(buf[i],HEX);DEBUGP(" ");
  }
 DEBUGPLN();
}
*/

void record(){
    /* Forever loop */
    uint8_t ch;
    uint16_t address=0;
    uint8_t length;
    if (SD.exists("record.bin"))
      SD.remove("record.bin");
    recordFile = SD.open("record.bin", FILE_WRITE);
    
  for (;;) {
    /* get character from UART */
    ch = getch();

    if(ch == Cmnd_STK_GET_PARAMETER) {
      unsigned char which = getch();
      if (!verifySpace()) break;
      if (which == 0x82) {
	/*
	 * Send optiboot version as "minor SW version"
	 */
	putch(OPTIBOOT_MINVER);
      } else if (which == 0x81) {
	  putch(OPTIBOOT_MAJVER);
      } else {
	/*
	 * GET PARAMETER returns a generic 0x03 reply for
         * other parameters - enough to keep Avrdude happy
	 */
	putch(0x03);
      }
    }
    else if(ch == Cmnd_STK_SET_DEVICE) {
      // SET DEVICE is ignored
      getNch(20);
    }
    else if(ch == Cmnd_STK_SET_DEVICE_EXT) {
      // SET DEVICE EXT is ignored
      getNch(5);
    }
    else if(ch == Cmnd_STK_LOAD_ADDRESS) {
      // LOAD ADDRESS
      uint16_t newAddress;
      newAddress = getch();
      newAddress = (newAddress & 0xff) | (getch() << 8);

      newAddress += newAddress; // Convert from word address to byte address
      address = newAddress;
      mybuf.pageaddress=address;
      if (!verifySpace())break;
    }
    else if(ch == Cmnd_STK_UNIVERSAL) {
      // UNIVERSAL command is ignored
      getNch(4);
      putch(0x00);
    }
    /* Write memory, length is big endian and is in bytes */
    else if(ch == Cmnd_STK_PROG_PAGE) {
      // PROGRAM PAGE - we support flash programming only, not EEPROM
      uint8_t *bufPtr;
      uint16_t addrPtr;

      getch();			/* getlen() */
      length = getch();
      getch();
      mybuf.size = length;
  // While that is going on, read in page contents
      bufPtr = buff;
      do *bufPtr++ = getch();
      while (--length);
      recordFile.write(buff,mybuf.size);
   
      // Read command terminator, start reply
      if (!verifySpace())break;
    }
    /* Read memory block mode, length is big endian.  */
    else if(ch == Cmnd_STK_READ_PAGE) {
      // READ PAGE - we only read flash
      getch();			/* getlen() */
      length = getch();
      getch();

      if (!verifySpace())break;
      recordFile.seek(address);
      recordFile.read(buff,length);
      Serial.write(buff,length);

    }

    /* Get device signature bytes  */
    else if(ch == Cmnd_STK_READ_SIGN) {
      // READ SIGN - return what Avrdude wants to hear
      verifySpace();
      putch(SIGNATURE_0);
      putch(SIGNATURE_1);
      putch(SIGNATURE_2);
    }
    else if (ch == 'Q') {
      verifySpace();
      recordFile.close();
      putch(Resp_STK_OK);
      return;
    }
    else if (ch == Cmnd_STK_LEAVE_PROGMODE){
      verifySpace();
      recordFile.close();
      putch(Resp_STK_OK);
      return;
    }
    else {
      // This covers the response to commands like STK_ENTER_PROGMODE
      verifySpace();
    }
    putch(Resp_STK_OK);
  }
  recordFile.close();
}
uint8_t getch() {
  while(!Serial.available());
  return Serial.read();
}
void readbytes(int n) {
  for (int x = 0; x < n; x++) {
    buff[x] = Serial.read();
  }
}
void getNch(uint8_t count) {
  do getch(); while (--count);
  verifySpace();
}
void putch(uint8_t ch){
  Serial.write(ch);
}

boolean verifySpace() {
  if (getch() != Sync_CRC_EOP) {
    return false;
  }
  putch(Resp_STK_INSYNC);
  return true;
}     

boolean isBinaryFile(File testfile){
  // call this before any reading...
  if (testfile.peek() == ':'){
    return 0;
  }
  else
    return 1;
}
  
     
