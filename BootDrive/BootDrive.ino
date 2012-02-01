#include <SoftwareSerial.h>
#include "stk500.h"
#include <SD.h>
/* Read hex file from sd/micro-sd card and program
 *  another arduino via ttl serial.
 * borrows heavily from avrdude source code (GPL license)
 *   Copyright (C) 2002-2004 Brian S. Dean <bsd@bsdhome.com>
 *   Copyright (C) 2008 Joerg Wunsch
 *  Created 12/26/2011 Kevin Osborn
 */
 

#define BOOT_BAUD 115200
#define txPin 2
#define rxPin 3
#define rstPin 7
SoftwareSerial sSerial= SoftwareSerial(rxPin,txPin);
// set up variables using the SD utility library functions:
SdFile root;
//chipselect for the wyolum i2sd is 10
const int chipSelect = 10;   

void setup() {
  //initialize serial port. Note that it's a good idea 
  // to use the softserial library here, so you can do debugging 
  // on USB. 
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(rstPin,OUTPUT);
  pinMode(chipSelect,OUTPUT);
    // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  digitalWrite(rstPin,HIGH);
  sSerial.begin(BOOT_BAUD);
  // and the regular serial port for error messages, etc.
  Serial.begin(BOOT_BAUD);
  unsigned int major=0;
  unsigned int minor=0;
  delay(100);
   toggle_Reset();
   delay(10);
   stk500_getsync();
   stk500_getparm(Parm_STK_SW_MAJOR, &major);
   sSerial.print("software major: ");
   sSerial.println(major);
  stk500_getparm(Parm_STK_SW_MINOR, &minor);
  sSerial.print("software Minor: ");
   sSerial.println(minor);
}

void loop() {
  // put your main code here, to run repeatedly: 
  
}


void toggle_Reset()
{
  digitalWrite(rstPin, LOW);
  delayMicroseconds(1000);
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
    sSerial.println("stk500_recv(): programmer is not responding\n");
    return -1;
  }
  return 0;
}
int stk500_drain()
{
sSerial.println("in Drain");
  while (Serial.available()> 0)
  {  
    sSerial.print("draining: ");
    sSerial.println(Serial.read(),HEX);
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
  
  stk500_send(buf, 2);
  stk500_drain();
  stk500_send(buf, 2);
  stk500_drain();
  
  stk500_send(buf, 2);
  if (stk500_recv(resp, 1) < 0)
    return -1;
  if (resp[0] != Resp_STK_INSYNC) {
        sSerial.print("stk500_getsync(): not in sync: resp: ");
        sSerial.println(resp[0],HEX);
    stk500_drain();
    return -1;
  }

  if (stk500_recv(resp, 1) < 0)
    return -1;
  if (resp[0] != Resp_STK_OK) {
    sSerial.print("stk500_getsync(): can't communicate with device: ");
    sSerial.println(resp[0],HEX);
 // add response code here...
    return -1;
  }
  return 0;
}
static int stk500_getparm(unsigned parm, unsigned * value)
{
  byte buf[16];
  unsigned v;
  int tries = 0;
sSerial.println("in getparm");
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
      sSerial.print("stk500_getparm(): can't get into sync\n");
      return -1;
    }
   if (stk500_getsync() < 0)
      return -1;
      
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    sSerial.print("stk500_getparm(): (a) protocol error, ");
    sSerial.println(buf[0],HEX);
    return -2;
  }

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  v = buf[0];

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_FAILED) {
    sSerial.print("stk500_getparm(): parameter ");
    sSerial.print(v);
    sSerial.println(" failed");
    return -3;
  }
  else if (buf[0] != Resp_STK_OK) {
    sSerial.print("stk500_getparm(): (a) protocol error, resp ");
    sSerial.println(buf[0],HEX);
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
    sSerial.println("memsize too small for sig byte read");
    return -1;
  }

  buf[0] = Cmnd_STK_READ_SIGN;
  buf[1] = Sync_CRC_EOP;

  stk500_send(buf, 2);

  if (stk500_recv(buf, 5) < 0)
    return -1;
  if (buf[0] == Resp_STK_NOSYNC) {
    sSerial.println("stk500_cmd(): in Read Sig programmer is out of sync\n");
	return -1;
  } else if (buf[0] != Resp_STK_INSYNC) {
    sSerial.print("arduino_read_sig_bytes(): (a) protocol expect=0x14 resp=");
    sSerial.print(buf[0],HEX);
	return -2;
  }
  if (buf[4] != Resp_STK_OK) {
    sSerial.print("arduino_read_sig_bytes(): (a) protocol error, "
			"expect=0x10, resp=");
    sSerial.println(buf[4],HEX);
    return -3;
  }

  m->buf[0] = buf[1];
  m->buf[1] = buf[2];
  m->buf[2] = buf[3];

  return 3;
}
static int stk500_is_page_empty(unsigned int address, int page_size, 
                                const unsigned char *buf)
{
    int i;
    for(i = 0; i < page_size; i++) {
        if(buf[address + i] != 0xFF) {
            /* Page is not empty. */
            return(0);
        }
    }
    
    /* Page is empty. */
    return(1);
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
      sSerial.println( "%s: stk500_loadaddr(): can't get into sync\n" );
      return -1;
    }
    if (stk500_getsync() < 0)
      return -1;
    goto retry;
  }
  else if (buf[0] != Resp_STK_INSYNC) {
    sSerial.println(
            "%s: stk500_loadaddr(): (a) protocol error, "
            "expect=0x%02x, resp=0x%02x\n");
    // maybe add more details later
    //             Resp_STK_INSYNC, buf[0]);
    return -1;
  }

  if (stk500_recv(buf, 1) < 0)
    exit(1);
  if (buf[0] == Resp_STK_OK) {
    return 0;
  }

  sSerial.println(
          "%s: loadaddr(): (b) protocol error, "
          "expect=0x%02x, resp=0x%02x\n"); 
    // maybe add more details later
    //             Resp_STK_INSYNC, buf[0]);

  return -1;
}
static int stk500_paged_write(AVRMEM * m, 
                              int page_size, int n_bytes)
{
  unsigned char buf[page_size + 16];
  int memtype;
  unsigned int addr;
  int a_div;
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
    //report_progress (addr, n_bytes, NULL);
    
    if ((addr + page_size > n_bytes)) {
	   block_size = n_bytes % page_size;
	}
	else {
	   block_size = page_size;
	}
  
    /* Only skip on empty page if programming flash. */
    if (flash) {
      if (stk500_is_page_empty(addr, block_size, m->buf)) {
          continue;
      }
    }
    tries = 0;
  retry:
    tries++;
    stk500_loadaddr(addr/a_div);

    /* build command block and avoid multiple send commands as it leads to a crash
        of the silabs usb serial driver on mac os x */
    i = 0;
    buf[i++] = Cmnd_STK_PROG_PAGE;
    buf[i++] = (block_size >> 8) & 0xff;
    buf[i++] = block_size & 0xff;
    buf[i++] = memtype;
    memcpy(&buf[i], &m->buf[addr], block_size);
    i += block_size;
    buf[i++] = Sync_CRC_EOP;
    stk500_send( buf, i);

    if (stk500_recv(buf, 1) < 0)
      exit(1);
    if (buf[0] == Resp_STK_NOSYNC) {
      if (tries > 33) {
        sSerial.println( "\n%s: stk500_paged_write(): can't get into sync\n");
        return -3;
      }
      if (stk500_getsync() < 0)
	return -1;
      goto retry;
    }
    else if (buf[0] != Resp_STK_INSYNC) {
      sSerial.println(
              "\n%s: stk500_paged_write(): (a) protocol error, "
              "expect=0x%02x, resp=0x%02x\n");
	      //maybe more detail later
	      //           Resp_STK_INSYNC, buf[0]);
      return -4;
    }
    
    if (stk500_recv(buf, 1) < 0)
      exit(1);
    if (buf[0] != Resp_STK_OK) {
      sSerial.println(
              "\n%s: stk500_paged_write(): (a) protocol error, "
              "expect=0x%02x, resp=0x%02x\n");
	      //maybe more detail later
	      //           Resp_STK_INSYNC, buf[0]);

      return -5;
    }
  }

  return n_bytes;
}
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
        sSerial.println( "\n%s: stk500_paged_load(): can't get into sync\n");
        return -3;
      }
      if (stk500_getsync() < 0)
	return -1;
      goto retry;
    }
    else if (buf[0] != Resp_STK_INSYNC) {
      sSerial.println(
              "\n%s: stk500_paged_load(): (a) protocol error, "
              "expect=0x%02x, resp=0x%02x\n");
	      //maybe more detail later
	      //           Resp_STK_INSYNC, buf[0]);
      return -4;
    }

    if (stk500_recv(&m->buf[addr], block_size) < 0)
      exit(1);

    if (stk500_recv(buf, 1) < 0)
      exit(1);

    if (buf[0] != Resp_STK_OK) {
        sSerial.println(
                "\n%s: stk500_paged_load(): (a) protocol error, "
                "expect=0x%02x, resp=0x%02x\n");
	      //maybe more detail later
	      //           Resp_STK_INSYNC, buf[0]);
        return -5;
      }
    }
  

  return n_bytes;
}




