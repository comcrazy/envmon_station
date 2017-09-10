/**************************************************************************/
/*!
@file     esp8266lib.cpp
@author   comcrazy
@license  GNU GPLv3

ESP8266 AT library

@section  HISTORY

v1.0 - First release
*/
/**************************************************************************/

#include "esp8266lib.h"

ESP8266Lib::ESP8266Lib(uint8_t rxpin, uint8_t txpin) {
  mySerial = new SoftwareSerial (rxpin, txpin); 
  mySerial->begin(9600);   
  mySerial->setTimeout(ESP01TIMEOUT);   
}

boolean ESP8266Lib::executeATCmd(String cmd, String ack, boolean halt_on_fail, int delayPeriod) {
#ifdef ESP01DEBUG
  Serial.print("CMD: [");
  Serial.print(cmd);
  Serial.println("]");
#endif

  mySerial->println(cmd);

  // Delay for command taking time
  delay(delayPeriod);

  if (!echoFind(ack))          // timed out waiting for ack string
    if (halt_on_fail) {          // If halt on failure
      errorHalt("Halt on no found  keyword [" + ack + "]");         // Critical failure halt.
    }
    else {
      return false;            // Let the caller handle it.
    }

  return true;                   // ack blank or ack found
}

boolean ESP8266Lib::echoFind(String keyword)
{
  memset(respMsg, 0, RESP_MESG_SIZE);
  byte current_char   = 0;

  byte keyword_length = keyword.length();
  int pos = 0;
  boolean gotKeyword = false;

  long deadline = millis() + ESP01TIMEOUT;              // Calculate timeout deadline
  while (millis() < deadline)                      // Try until deadline
  {
    while (mySerial->available())                        // If characters are available
    {
      respMsg[pos] = mySerial->read();
      pos++;
    }
  }

#ifdef ESP01DEBUG
  Serial.print("RESP");
  if (pos != 0) {
    Serial.print(": [");
  } else {
    Serial.print(": [N/A");
  }
#endif

  if (keyword_length != 0 && pos != 0) {
    for (int counter = 0; counter < pos; counter++) {

#ifdef ESP01DEBUG
      if ((respMsg[counter] >= 32 && respMsg[counter] <= 126) || respMsg[counter] == 0x0a || respMsg[counter] == 0x0d) {
        Serial.print((char)respMsg[counter]);
      } else {
        if (respMsg[counter] < 0x10) {
          Serial.print("0x");
          Serial.print(respMsg[counter], HEX);
        } else {
          Serial.print(respMsg[counter], HEX);
        }
      }
#endif

      if ((char)respMsg[counter] == keyword[current_char])
      {
        if (++current_char == keyword_length) {

          gotKeyword = true;
        }
      }
    }
  } else {
    gotKeyword = true;
  }

#ifdef ESP01DEBUG
  Serial.println("]");
#endif

  return gotKeyword;
}

void ESP8266Lib::echoFlush()
{
  while (mySerial->available()) mySerial->read();
}

void ESP8266Lib::errorHalt(String message)
{
#ifdef ESP01DEBUG
  Serial.print("System halt:");
  Serial.println(message);
#endif
  while (true) {};
}

void ESP8266Lib::serialCommandMode() {

  char c;
  if (Serial.available())
  {
    c = Serial.read();
    mySerial->write(c);
  }
  if (mySerial->available())
  {
    c = mySerial->read();
    Serial.write(c);
  }
}

