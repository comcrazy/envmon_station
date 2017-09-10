/**************************************************************************/
/*!
@file     ML8511.cpp
@author   comcrazy
@license  GNU GPLv3

First version of an Arduino Library for the ML8511 uv sensor

@section  HISTORY

v1.0 - First release
*/
/**************************************************************************/

#include "ML8511.h"

/**************************************************************************/
/*!
@brief  Default constructor

@param[in] pin  The analog input pin for the readout of the sensor
*/
/**************************************************************************/

ML8511::ML8511(uint8_t pin, uint8_t ref3v3_pin) {
  _pin = pin;
  _ref3v3_pin = ref3v3_pin;
}

float ML8511::getReading()
{
  int uvLevel = averageAnalogRead(_pin);
  int refLevel = averageAnalogRead(_ref3v3_pin);

  //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
  float outputVoltage = 3.3 / refLevel * uvLevel;

  float uvIntensity = mapfloat(outputVoltage); //Convert the voltage to a UV intensity level

  return uvIntensity;
}

//Takes an average of readings on a given pin
//Returns the average
int ML8511::averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0;

  for (int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
    
  runningValue /= numberOfReadings;

  return (runningValue);
}

//The Arduino Map function but for floats
//From: http://forum.arduino.cc/index.php?topic=3922.0
float ML8511::mapfloat(float x)
{
  return (x - IN_MIN) * (OUT_MAX - OUT_MIN) / (IN_MAX - IN_MIN) + OUT_MIN;
}
