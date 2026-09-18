#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <VL53L0X.h>

const char *ssid = "RadioGattoMaurizio 2.4";
const char *password = "trastai-3";
const unsigned long WIFI_TIMEOUT_MS = 15000;

VL53L0X sensor;

void connectWiFi()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WiFi connection failed.");
    Serial.println("Waiting before retry...");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  // pinMode(D3, OUTPUT);
  // digitalWrite(D3, HIGH);
  // delay(10);
  Wire.begin(D2, D1); // SDA, SCL
  Serial.println("Scanning I2C bus...");

  byte count = 0;
  for (byte addr = 1; addr < 127; addr++)
  {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
    {
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
      count++;
    }
  }
  if (count == 0)
    Serial.println("No I2C devices found.");
  // Wire.setClockStretchLimit(15000); // VL53L0X stretches the clock during ranging; ESP8266's default limit is too short
  // sensor.setTimeout(1500);
  // if (!sensor.init())
  // {
  //   Serial.println("Failed to detect VL53L0X!");
  //   while (1)
  //   {
  //   }
  // }

  Serial.println("VL53L0X ready.");
  // sensor.startContinuous();

  // WiFi.mode(WIFI_STA);
  // connectWiFi();
}

void loop()
{
  // if (WiFi.status() != WL_CONNECTED)
  // {
  //   connectWiFi();
  // }
  // int distance = sensor.readRangeContinuousMillimeters();

  // if (sensor.timeoutOccurred())
  // {
  //   Serial.println("TIMEOUT");
  // }
  // else
  // {
  //   Serial.print("Distance: ");
  //   Serial.print(distance);
  //   Serial.println(" mm");
  // }

  delay(100);
  // delay(1000);
}