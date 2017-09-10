/**************************************************************************/
/*!
@file     ML8511.h
@author   comcrazy
@license  GNU GPLv3

First version of an Arduino Library for the ML8511 uv sensor

@section  HISTORY

v1.0 - First release
*/
/**************************************************************************/
#ifndef ML8511_H
#define ML8511_H
#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#define IN_MIN 0.99
#define IN_MAX 2.8
#define OUT_MIN 0.0
#define OUT_MAX 15.0

class ML8511 {
 private:
  uint8_t _pin;
  uint8_t _ref3v3_pin;
  int averageAnalogRead(int pinToRead);
  float mapfloat(float x);
  
 public:
  ML8511(uint8_t pin, uint8_t ref3v3_pin);
  float getReading();    
};
#endif
