#! /usr/bin/env python
#coding=utf-8

#                   Adafinger: Library and command line utility for using the Adafruit Fingerprint sensor on a PC
#                      requires the pyserial module
#                      Based on Arduino Library by Limor Fried
#                      License: Creative Commons Attribution
#Author: Kevin Osborn
import serial
import time
import signal
import sys

FINGERPRINT_OK=0x00
FINGERPRINT_PACKETRECIEVEERR=0x01
FINGERPRINT_NOFINGER=0x02
FINGERPRINT_IMAGEFAIL=0x03
FINGERPRINT_IMAGEMESS=0x06
FINGERPRINT_FEATUREFAIL=0x07
FINGERPRINT_NOMATCH=0x08
FINGERPRINT_NOTFOUND=0x09
FINGERPRINT_ENROLLMISMATCH=0x0A
FINGERPRINT_BADLOCATION=0x0B
FINGERPRINT_DBRANGEFAIL=0x0C
FINGERPRINT_UPLOADFEATUREFAIL=0x0D
FINGERPRINT_PACKETRESPONSEFAIL=0x0E
FINGERPRINT_UPLOADFAIL=0x0F
FINGERPRINT_DELETEFAIL=0x10
FINGERPRINT_DBCLEARFAIL=0x11
FINGERPRINT_PASSFAIL=0x13
FINGERPRINT_INVALIDIMAGE=0x15
FINGERPRINT_FLASHERR=0x18
FINGERPRINT_INVALIDREG=0x1A
FINGERPRINT_ADDRCODE=0x20
FINGERPRINT_PASSVERIFY=0x21

FINGERPRINT_STARTCODE=0xEF01

FINGERPRINT_COMMANDPACKET=0x1
FINGERPRINT_DATAPACKET=0x2
FINGERPRINT_ACKPACKET=0x7
FINGERPRINT_ENDDATAPACKET=0x8

FINGERPRINT_TIMEOUT=0xFF
FINGERPRINT_BADPACKET=0xFE

FINGERPRINT_GETIMAGE=0x01
FINGERPRINT_IMAGE2TZ=0x02
FINGERPRINT_REGMODEL=0x05
FINGERPRINT_STORE=0x06
FINGERPRINT_VERIFYPASSWORD=0x13
FINGERPRINT_HISPEEDSEARCH=0x1B
class adafinger(object):



    thePassword= 0
    theAddress=0xFFFFFFFF
    def __init__(self, port, baudrate=57600):
        self._port=port
        self._baudrate =baudrate
        self._serial = None
        self.fingerID = -1
        self.confidence = -1
        try:
            self.serial = serial.serial_for_url(port, baudrate, parity='N',timeout=1)
        except AttributeError:
            # happens when the installed pyserial is older than 2.5. use the
            # Serial class directly then.
            self.serial = serial.Serial(port, baudrate, parity='N', timeout=1)

    def getImage(self):
        imagePacket = [FINGERPRINT_GETIMAGE]
        self.writePacket(self.theAddress,FINGERPRINT_COMMANDPACKET, imagePacket, 3)
        (len, reply) = self.getReply()
        if len >= FINGERPRINT_BADPACKET:
            return -1
        if (len !=1) and (reply[0] != chr(FINGERPRINT_ACKPACKET)):
            return -1
        return ord(reply[1])

    def image2Tz(self,slot=1):
        imPacket = [FINGERPRINT_IMAGE2TZ,slot]
        self.writePacket(self.theAddress,FINGERPRINT_COMMANDPACKET, imPacket, 4)
        (len, reply) = self.getReply()
        if len >= FINGERPRINT_BADPACKET:
            return -1
        if (len !=1) and (reply[0] != chr(FINGERPRINT_ACKPACKET)):
            return -1
        return ord(reply[1])

    def fingerFastSearch(self):
        spacket = [FINGERPRINT_HISPEEDSEARCH, 0x01,0x00,0x00,0x00,0xA3]
        self.writePacket(self.theAddress, FINGERPRINT_COMMANDPACKET,spacket,8)
        (len,reply) = self.getReply()
 #       for x in reply:
 #           print "%02X:" % ord(x)
        if len >= FINGERPRINT_BADPACKET:
            return -1
        if (len !=1) and (reply[0] != chr(FINGERPRINT_ACKPACKET)):
            return -1
        self.fingerID = ord(reply[2])<<8
        self.fingerID = self.fingerID | ord(reply[3])

        self.confidence = ord(reply[4])<< 8
        self.confidence = self.confidence | ord(reply[5])

        return ord(reply[1])
                                

    def verifyPassword(self):
        passpacket = [FINGERPRINT_VERIFYPASSWORD,
                      (self.thePassword >> 24), (self.thePassword >> 16), (self.thePassword >>8), self.thePassword
                      ]
        reply = []
        self.writePacket(0xFFFFFFFF, FINGERPRINT_COMMANDPACKET,passpacket,7)
        (len, reply)  = self.getReply()
#        print "got packet len %d\n" % (len)
        if (len == 1)  and (reply[0] == chr(FINGERPRINT_ACKPACKET)) and (reply[1] == chr(FINGERPRINT_OK)):
#            print "got packet type: %02X\n" % reply[0]
            return True
        else:
            return False

    def writePacket(self,address, packettype, packet, len):
        self.serial.write(chr((FINGERPRINT_STARTCODE >>8)&0x0ff))
        self.serial.write(chr(FINGERPRINT_STARTCODE & 0x0ff))
        self.serial.write(chr((address >>24)&0xff))
        self.serial.write(chr((address>>16)&0xff))
        self.serial.write(chr((address >>8)&0xff))
        self.serial.write(chr(address & 0x0ff))
        self.serial.write(chr(packettype&0x0ff))
        self.serial.write(chr((len >>8)&0xff))
        self.serial.write(chr(len&0x0ff))

        sum = ((len >>8)&0x0ff) + (len &0x0ff) + packettype
        for i in range(len-2):
            self.serial.write(chr(packet[i]&0x0ff))
            sum = sum + packet[i]   # the incoming packet are all int types. not sure if this will work for all packets
#            print "checksum: %02X" % sum
        self.serial.write(chr((sum >> 8)&0x0ff))
        self.serial.write(chr(sum&0x0ff))

    def getReply(self,timeout=10 ):
        reply=[]
        returnpacket =[]
        len =0
        idx = 0
        timer =0
        while (True):
            while (self.serial.inWaiting() == 0):
                time.sleep(.1)
  #              print "tick timout = %d timer = %d\n" % (timeout, timer)
                timer = timer+1
                if timer >= timeout:
                    return FINGERPRINT_TIMEOUT, reply
            reply.append(self.serial.read(1))
            if (idx ==0) and reply[0] != chr(FINGERPRINT_STARTCODE >> 8):
                print "error: first char not FINGERPRINT STARTCODE\n"
                print "%02X\n" % (ord(reply[0]))
                continue
            idx = idx + 1
            # check packet
            if idx >=9:
                if (reply[0] !=chr(FINGERPRINT_STARTCODE>>8)) or (reply[1] != chr(FINGERPRINT_STARTCODE &0xFF)):
                    print "error: BADPACKET\n"
                    return FINGERPRINT_BADPACKET, reply
                packettype = reply[6]
                len = ord(reply[7])
                len = len << 8
                len = len | ord(reply[8])
                len = len -2
                if (idx <= (len+10)):
                    continue
#                print "whole packet\n"
#                for c in reply:
#                    print "%02X:" % ord(c)
                returnpacket.append(packettype)
#                print "return packet: %02X:" % ord(packettype)
                for i in range(len):
                    returnpacket.append(reply[9+i])
#                    print "%02X:" % ord(reply[9+i])
                return (len, returnpacket)
        # returns -1 on error, otherwise returns fingerprint ID #
    def getFingerPrintID(self):
            fprint = -1
            fprint = self.getImage()
            if fprint != FINGERPRINT_OK:
 #               print "failed to get image"
                return -1
            fprint = self.image2Tz()
            if fprint != FINGERPRINT_OK:
 #               print "failed to convert image"
                return -1
            fprint = self.fingerFastSearch()
            if fprint != FINGERPRINT_OK:
  #              print "failed to find finger"
                return -1
            return self.fingerID

        
def main():
    import optparse, sys
    parser = optparse.OptionParser(
        usage = "%prog [options] [port]",
        description = "Adafinger: command line interface to Adafruit fingerprint reader"
    )
    parser.add_option("-p", "--port",
        dest = "port",
        help = "port, a number or a device name",
        default = 'COM1'
    )
    parser.add_option("-e", "--enroll",
        dest = "enrollslot",
        action = "store",
        type = 'int',
        default = -1,
        help = "enrollslot, a numbered slot for fingerprint enroll"
    )
    parser.add_option("-s", "--scan",
        dest ="scan",
        action = "store_true",
        help ="scan for fingerprint, return slot if detected",
        default = False
    )
    (options, args) = parser.parse_args()
    port = options.port
    baudrate = 57600
    if args:
        if options.port is not None:
            parser.error("no arguments are allowed, options only when --port is given")
        port = args.pop(0)
    finger = adafinger(port)
    if not finger.verifyPassword():
        print "no reader found"
        sys.exit(1)
    else:
        print "reader found!"
        if options.scan:
            while True:
                finger.getFingerPrintID()
                if finger.fingerID != -1:
                    print "Found ID ID: %d with confidence of %d" % (finger.fingerID, finger.confidence)
                    break
            sys.exit(0)

def ctlc_handler(signal,frame):
    sys.exit(0)
signal.signal(signal.SIGINT, ctlc_handler)

if __name__ == '__main__':
    main()
