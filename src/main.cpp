#include <Arduino.h>
#include <core/YoloDevice.h>


void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("====== YOLO ======");
  YoloDevice::instance().init();
}


void loop()
{
  YoloDevice::instance().update();
  delay(100);
}