#include <Arduino.h>
#include <core/YoloDevice.h>

#define BATTERY_PIN 0
void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("====== YOLO ======");
  YoloDevice::instance().init("bleserver");

  pinMode(BATTERY_PIN, INPUT);
  // TODO byteParameter
}


void loop()
{
  YoloDevice::instance().update();
  delay(100);
}