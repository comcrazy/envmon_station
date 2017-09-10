#include <SoftwareSerial.h>
#include "dht.h"
#include "MQ135.h"
#include "ML8511.h"
#include "soundSensor.h"
#include "esp8266lib.h"

#define SSID        "ciot"
#define DEST_HOST   "192.168.250.1"
#define DEST_PORT   "80"
#define URI         "GET /envmonhub/pushData.php?payload="

#define DHT11_DPIN 5
#define AIR_QUALITY_APIN A5
#define SOUND_APIN A3
#define UV_APIN A1
#define REF_3V3_APIN A0
#define REF_5V_APIN A2

#define DHT_READ_INTERVAL 2000
#define SENSORS_READ_INTERVAL 180000 //milliseconds
#define DEVICE_ONE 0x1
#define DEVICE_TWO 0x2
#define SOFTSERIAL_RX_PIN 7
#define SOFTSERIAL_TX_PIN 8
#define PAYLOAD_SIZE 16

ESP8266Lib esp8266lib (SOFTSERIAL_RX_PIN, SOFTSERIAL_TX_PIN);
dht DHT;
MQ135 airSensor = MQ135(AIR_QUALITY_APIN);
ML8511 uvSensor = ML8511(UV_APIN, REF_3V3_APIN);
SoundSensor soundSensor = SoundSensor(SOUND_APIN);

float airValue;
float soundValue;
float uvValue;
byte deviceID;
unsigned long lastDHTReadingTimestamp;
unsigned long lastSensorReadingTimestamp;
byte respMsg[128];

void setup() {
  pinMode(DHT11_DPIN, INPUT);
  pinMode(AIR_QUALITY_APIN, INPUT);
  pinMode(SOUND_APIN, INPUT);
  pinMode(UV_APIN, INPUT);
  pinMode(REF_3V3_APIN, INPUT);
  pinMode(REF_5V_APIN, INPUT);

  deviceID = DEVICE_ONE;
  lastDHTReadingTimestamp = 0;
  lastSensorReadingTimestamp = 0;

  airValue = 0;
  soundValue = 0;
  uvValue = 0;
  Serial.begin(9600);

  Serial.print("Health Sensor Station ID :");
  Serial.println(deviceID);
  Serial.print("DHT LIBRARY VERSION: ");
  Serial.println(DHT_LIB_VERSION);
  Serial.println();

  Serial.println("=============== Setup Start ===============");

  Serial.println("--------------- DHT Calibrating ---------------");
  DHTReading();
  Serial.println("--------------- Air Sensor Calibrating ---------------");
  airSensor.setRZero(airSensorCalibrating());
  lastDHTReadingTimestamp = 0;

  Serial.println("--------------- ESP-01 module setup ---------------");
  esp8266lib.echoFlush();
  esp8266lib.executeATCmd("ATE0", "OK", CONTINUE, 50);
  esp8266lib.executeATCmd("AT+CWJAP?", SSID, HALT, 50);
  esp8266lib.executeATCmd("AT+CIPSTA_CUR?", ":ip:", CONTINUE, 50);

  Serial.println("=============== Setup END ===============");


}

void loop() {
  //esp8266lib.serialCommandMode();
  //return;
  //Sound db, get the max within interval
  float tmpSoundValue = soundSensor.measureVolume();
  soundValue = tmpSoundValue > soundValue ? tmpSoundValue : soundValue;

  if (lastSensorReadingTimestamp == 0 || millis() - lastSensorReadingTimestamp >= SENSORS_READ_INTERVAL) {
    byte payload[PAYLOAD_SIZE];
    unsigned long tranStart = millis();

    memset(payload, 0, PAYLOAD_SIZE);
    readSensor();
    payloadComposing(payload, PAYLOAD_SIZE);
    printSensorReading(payload, PAYLOAD_SIZE);
    sendPayload(payload, PAYLOAD_SIZE);

    soundValue = 0;
    Serial.print("Transaction take ");
    Serial.print(millis() - tranStart);
    Serial.println("ms");
  }
}

void sendPayload(byte bytePayload[], int payloadSize) {

  char stringPayload[(PAYLOAD_SIZE * 2) + 1];
  memset(stringPayload, 0, (PAYLOAD_SIZE * 2) + 1);
  byteToHexString(bytePayload, payloadSize, stringPayload);

  Serial.print("sent payload [");
  Serial.print(stringPayload);
  Serial.println("]");

  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += DEST_HOST;
  cmd += "\",";
  cmd += DEST_PORT; // Establish TCP connection

  if (!esp8266lib.executeATCmd(cmd, "OK", CONTINUE, 50)) {
    return;
  }

  if (!esp8266lib.executeATCmd("AT+CIPSTATUS", "STATUS:3", CONTINUE, 50)) {
    return; // Get connection status
  }

  // Build HTTP request.
  String cmd2 = URI;
  cmd2 += stringPayload;
  cmd2 += "\r\n";

  // Ready the module to receive raw data
  if (!esp8266lib.executeATCmd("AT+CIPSEND=" + String(cmd2.length()), ">", CONTINUE, 50)) {
    esp8266lib.executeATCmd("AT+CIPCLOSE", "OK", CONTINUE, 50);
    return;
  }

  esp8266lib.executeATCmd(cmd2, "SEND OK", CONTINUE, 50); // Send the raw HTTP request
  //esp8266lib.executeATCmd("AT+CIPCLOSE", "OK", CONTINUE, 50);
}


void printSensorReading(byte payload[], int payloadSize) {

  Serial.print("H:");
  Serial.print(DHT.humidity, 0);
  Serial.print("%,\tT:");
  Serial.print(DHT.temperature, 2);
  Serial.print("C,\tA:");
  Serial.print(airValue);
  Serial.print("PPM,\tU:");
  Serial.print(uvValue);
  Serial.print("mW/cm^2,\tS:");
  Serial.print(soundValue, DEC);
  Serial.print("dB,\tpayload:");

  for (int counter = 0; counter < payloadSize; counter++)
  {
    if (payload[counter] < 0x10) {
      Serial.print("0");
    }
    Serial.print(payload[counter], HEX);
  }
}

void readSensor() {
  if (millis() - lastDHTReadingTimestamp >= DHT_READ_INTERVAL) {
    DHTReading();
    lastDHTReadingTimestamp = millis();
  }

  airValue = airSensor.getCorrectedPPM((float)DHT.temperature, (float)DHT.humidity);
  //airValue = airSensor.getPPM();
  uvValue = uvSensor.getReading();
  lastSensorReadingTimestamp  = millis();
}

void byteToHexString(byte bytePayload[], int bytePayloadLen, char stringPayload[]) {
  int pos = 0;

  for (int counter = 0;  counter < bytePayloadLen; counter++) {
    int currUpper = bytePayload[counter] >> 4 & 0xF;
    stringPayload[pos] = fourBitToChar(currUpper);
    pos++;
    int currLower =  bytePayload[counter] & 0xF;
    stringPayload[pos] = fourBitToChar(currLower);
    pos++;

  }
  stringPayload[pos] = 0;
}

char fourBitToChar(int curr4Bit) {
  char buffer = 0;
  switch (curr4Bit) {
    case 0:
      buffer = '0';
      break;
    case 1:
      buffer = '1';
      break;
    case 2:
      buffer = '2';
      break;
    case 3:
      buffer = '3';
      break;
    case 4:
      buffer = '4';
      break;
    case 5:
      buffer = '5';
      break;
    case 6:
      buffer = '6';
      break;
    case 7:
      buffer = '7';
      break;
    case 8:
      buffer = '8';
      break;
    case 9:
      buffer = '9';
      break;
    case 10:
      buffer = 'A';
      break;
    case 11:
      buffer = 'B';
      break;
    case 12:
      buffer = 'C';
      break;
    case 13:
      buffer = 'D';
      break;
    case 14:
      buffer = 'E';
      break;
    case 15:
      buffer = 'F';
      break;
  }

  return buffer;
}


void payloadComposing(byte payload[], int payloadSize) {
  payload[0] = 0x55; // header
  payload[1] = 0xAA; // header

  payload[2] = 11; // payload length
  payload[3] = deviceID; // device ID

  short humidityVal = (short) DHT.humidity;
  byte b1 = humidityVal >> 8 & 0xFF;
  byte b2 = humidityVal & 0xFF;
  payload[4] = b1;
  payload[5] = b2;

  short tempVal = (short) (DHT.temperature * 10);
  byte b3 = tempVal >> 8 & 0xFF;
  byte b4 = tempVal & 0xFF;
  payload[6] = b3;
  payload[7] = b4;

  short airVal = (short) airValue;
  byte b5 = airVal >> 8 & 0xFF;
  byte b6 = airVal & 0xFF;
  payload[8] = b5;
  payload[9] = b6;

  short soundVal = (short) soundValue * 10;
  byte b7 = soundVal >> 8 & 0xFF;
  byte b8 = soundVal & 0xFF;
  payload[10] = b7;
  payload[11] = b8;

  short uvVal = (short) (uvValue * 100);
  byte b9 = uvVal >> 8 & 0xFF;
  byte b10 = uvVal & 0xFF;
  payload[12] = b9;
  payload[13] = b10;

  byte checksum = deviceID + b1 + b2 + b3 + b4 + b5 + +b6 + b7 + b8 + b9 + b10;
  payload[14] = checksum & 0xFF; //checksum
  payload[15] = 0xFF; //footer
}

float airSensorCalibrating() {
  double total = 0;
  int calCounter = 0;
  float rzeroResult = 0;
  int taken = 100;

  Serial.print("Air Sensor Calibrating:");
  for (int counter = 0; counter < taken; counter++) {
    if (counter % 2 == 0 && counter != 0) {
      DHTReading();
    }

    if (counter % 10 == 0) {
      Serial.print((float)counter / taken * 100, 0);
      Serial.print("%");
      Serial.print(",");
    }

    float currReading = airSensor.getCorrectedRZero((float)DHT.temperature, (float)DHT.humidity);
    //float currReading = airSensor.getRZero();

    if (counter >= 50) {
      total += currReading;
      calCounter++;
    }
    delay(1000);
  }
  Serial.print("100%");
  Serial.println();

  rzeroResult = (float) total / calCounter;
  Serial.print("RZERO is:");
  Serial.println(rzeroResult);

  return rzeroResult;
}

void DHTReading() {
  int chk = DHT.read11(DHT11_DPIN);
  switch (chk)
  {
    case DHTLIB_OK:
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("DHT Checksum error");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("DHT Time out error");
      break;
    default:
      Serial.println("DHT Unknown error");
      break;
  }
}








