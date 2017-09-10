/**************************************************************************/
/*!
@file     esp8266lib.h
@author   comcrazy
@license  GNU GPLv3

ESP8266 AT library

@section  HISTORY

v1.0 - First release
*/
/**************************************************************************/
#ifndef ESP8266LIB_H
#define ESP8266LIB_H
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <SoftwareSerial.h>

#define ESP01TIMEOUT     500
#define CONTINUE    false
#define HALT        true
#define RESP_MESG_SIZE 128
#define ESP01DEBUG 1

class ESP8266Lib {
 private:
  SoftwareSerial *mySerial;
  byte respMsg[RESP_MESG_SIZE];

  boolean echoFind(String keyword);
  void errorHalt(String message);
  
 public:
  ESP8266Lib(uint8_t rxpin, uint8_t txpin);
  boolean executeATCmd(String cmd, String ack, boolean halt_on_fail, int delayPeriod);
  void echoFlush();
  void serialCommandMode();  
};

/*
 * ATE0
AT+SLEEP=2
AT+CWMODE_DEF=1
AT+CWJAP_DEF="ciot","comcrazyiot1001"
AT+CWAUTOCONN=1
 */
#endif
