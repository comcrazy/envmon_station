#include <SoftwareSerial.h>
#include "dht.h"
#include "MQ135.h"
#include "ML8511.h"
#include "soundSensor.h"
#include "esp8266lib.h"

#define SSID        "ciot"
#define DEST_HOST   "192.168.250.1"
#define DEST_PORT   "80"
#define URI         "GET /pushData.php?"

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
    unsigned long tranStart = millis();
    readSensor();
    printSensorReading();
    sendPayload();

    soundValue = 0;
    Serial.print("Transaction take ");
    Serial.print(millis() - tranStart);
    Serial.println("ms");
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

void printSensorReading() {

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
}

void sendPayload() {
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
  cmd2 += "soundlevel=";
  cmd2 += soundValue;
  cmd2 += "&";
  cmd2 += "uvlevel=";
  cmd2 += uvValue;
  cmd2 += "&";  
  cmd2 += "templevel=";
  cmd2 += DHT.temperature;
  cmd2 += "&"; 
  cmd2 += "humiditylevel=";
  cmd2 += DHT.humidity;
  cmd2 += "&";   
  cmd2 += "airqality=";
  cmd2 += airValue;
  cmd2 += "&";  
  cmd2 += "\r\n";

  // Ready the module to receive raw data
  if (!esp8266lib.executeATCmd("AT+CIPSEND=" + String(cmd2.length()), ">", CONTINUE, 50)) {
    esp8266lib.executeATCmd("AT+CIPCLOSE", "OK", CONTINUE, 50);
    return;
  }

  esp8266lib.executeATCmd(cmd2, "SEND OK", CONTINUE, 50); // Send the raw HTTP request
  //esp8266lib.executeATCmd("AT+CIPCLOSE", "OK", CONTINUE, 50);
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








