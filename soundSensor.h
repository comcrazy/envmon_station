/**************************************************************************/
/*!
  @file     soundSensor.h
  @author   comcrazy
  @license  GNU GPLv3

  First version of an Arduino Library for the sound sensor using LM393

  @section  HISTORY

  v1.0 - First release
*/
/**************************************************************************/
#ifndef SOUNDSENSOR_H
#define SOUNDSENSOR_H
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define SAMPLE_WINDOW 50
#define SAMPLE_COUNT 5

class SoundSensor {
  private:
    uint8_t _apin;

  public:
    SoundSensor(uint8_t apin);
    float measureVolume();
};
#endif
