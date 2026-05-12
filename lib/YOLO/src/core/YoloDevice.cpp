#include "YoloDevice.h"

YoloDevice::YoloDevice() : Component("yolo")
{
  bleModule = new BLEModule();
  bleModule->onControl([this](const uint8_t* pData, size_t length){
    Serial.printf("control notification\n");
  //  if (length == sizeof(int)) {
  //       int value;
  //       memcpy(&value, pData, sizeof(int));
  //       Serial.printf("Notification int: %d\n", value);
  //   } 
  //   else if (length == sizeof(float)) {
        float value;
        memcpy(&value, pData, sizeof(float));
        // Serial.printf("Notification float: %f\n", value);
    // } 
    // else if (length == sizeof(uint8_t)) {
    //     uint8_t value;
    //     memcpy(&value, pData, sizeof(uint8_t));
    //     Serial.printf("Notification uint8: %d\n", value);
    // }
    // else {
    //     Serial.printf("Unexpected length: %d\n", length);
    // }
  });
  modules.emplace_back(bleModule);
  modules.emplace_back(new ServoModule());
}

void YoloDevice::init(String config)
{
  FileManager::init();

  if (!config.isEmpty())
    FileManager::setNewConfig(config);

  Serial.println();
  Serial.println();

  File configFile = FileManager::openConfigFile();
  if (!configFile)
  {
    Serial.println("no config file ! Please upload LittleFS image");

    // register led and launch AP
  }
  else
  {
    Serial.println();
    JsonDocument json;
    DeserializationError error = deserializeJson(json, configFile);
    if (error)
      Serial.println("failed to deserialize json config : " + String(error.c_str()));
    else
      for (auto m : modules)
        m->loadConfig(json[m->getName()].as<JsonObject>());

      for (auto m : modules)
        Serial.println(m->getName());
    // FileManager::printWifiCredentials();
    Serial.println();
    Serial.println("INIT OK: " + FileManager::getCurrentConfigNiceName());
    Serial.println();
    Serial.println();

    for (auto module : modules)
      for (auto comp : module->getComponents())
        for (auto param : comp->getParameters())
          param->setChangeCallback(onParamChangedStatic, this);
    
    bleModule->setupServer(*this);

    // if(bleModule->isInitialized())
    //   bleModule->initService(configCBOR, len);
  }
}

void YoloDevice::update()
{
  for (auto m : modules)
    m->update();

}

void YoloDevice::onParamChanged(Parameter *p)
{
  dbg("param changed: %s", p->getName());
  // Only notify for readable parameters
  if (p->getAccess() == ParameterAccess::WRITE_ONLY)
    return;

  // bleModule->notify(p);
}

