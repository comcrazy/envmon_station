/**************************************************************************/
/*!
  @file     soundSensor.cpp
  @author   comcrazy
  @license  GNU GPLv3

  First version of an Arduino Library for the sound sensor using LM393

  @section  HISTORY

  v1.0 - First release
*/
/**************************************************************************/

#include <math.h>
#include "soundSensor.h"

/**************************************************************************/
/*!
  @brief  Default constructor

  @param[in] pin  The analog input pin for the readout of the sensor
*/
/**************************************************************************/

SoundSensor::SoundSensor(uint8_t apin) {
  _apin = apin;
}


float SoundSensor::measureVolume()
{
  double lowestVol = 0;

  for (int counter = 0; counter < SAMPLE_COUNT; counter++) {
    unsigned long startMillis = millis(); // Start of sample window
    unsigned int peakToPeak = 0;   // peak-to-peak level

    unsigned int signalMax = 0;
    unsigned int signalMin = 1024;

    unsigned int sample;

    // collect data for 50 mS
    while (millis() - startMillis < SAMPLE_WINDOW)
    {
      sample = analogRead(_apin);
      if (sample < 1024)  // toss out spurious readings
      {
        if (sample > signalMax)
        {
          signalMax = sample;  // save just the max levels
        }
        else if (sample < signalMin && sample)
        {
          signalMin = sample;  // save just the min levels
        }
      }
    }
    peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude

    double volts = ((peakToPeak * 3.3) / 1024) * 0.707;  // convert to RMS voltage
    double rawDb = log10(volts / 0.00631) * 20;
    double refactorDb = rawDb + 94 - 44 - 25;

    if (counter == 0) {
      lowestVol = refactorDb;
    } else {
      lowestVol = refactorDb < lowestVol ? refactorDb : lowestVol;
    }
  }
  /*Serial.print(signalMax);
    Serial.print(":");
    Serial.print(signalMin);
    Serial.print(":");
    Serial.print(peakToPeak);
    Serial.print(":");
    Serial.print(volts);
    Serial.print(":");
    Serial.print(rawDb);
    Serial.print(":");
    Serial.println(lowestVol);*/
  return lowestVol;
}

