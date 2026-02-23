#include "YoloDevice.h"

YoloDevice::YoloDevice() : Component("yolo")
{
  bleModule = new BLEModule();
  modules.emplace_back(bleModule);

  modules.emplace_back(new I2CModule());
  // modules.emplace_back(wifiModule);
  // modules.emplace_back(ledModule);
  modules.emplace_back(new ServoModule());
  // modules.emplace_back(gpioModule);
  // modules.emplace_back(odriveModule);
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

    // FileManager::printWifiCredentials();
    Serial.println();
    Serial.println("INIT OK: " + FileManager::getCurrentConfigNiceName());
    Serial.println();
    Serial.println();

    for (auto module : modules)
      for (auto comp : module->getComponents())
        for (auto param : comp->getParameters())
          param->setChangeCallback(onParamChangedStatic, this);
    
    bleModule->postInit(*this);

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
  if (p->getAccess() == ParamAccess::WRITE_ONLY)
    return;

  // bleModule->notify(p);
}


std::pair<uint8_t*, size_t> YoloDevice::buildConfigCBOR()
{
    JsonDocument doc;

    doc["device"] = FileManager::getCurrentConfigNiceName().c_str();

    JsonArray modulesArr = doc["modules"].add<JsonArray>();
    for (auto m : modules)
    {
      JsonObject mObj = modulesArr.add<JsonObject>();
      mObj["id"] = m->getModuleID();
      mObj["name"] = m->getName();

      JsonArray paramsArr = mObj["parameters"].add<JsonArray>();
      for (auto p : m->getParameters())
      {

        JsonObject pObj = paramsArr.add<JsonObject>();
        pObj["name"] = p->getName();
        pObj["type"] = (int)p->getType();
        pObj["access"] = (int)p->getAccess();
      }

      JsonArray compsArr = mObj["components"].add<JsonArray>();

      for (auto c : m->getComponents())
      {

        JsonObject cObj = compsArr.add<JsonObject>();
        cObj["name"] = c->getName();

        JsonArray paramsArr = cObj["parameters"].add<JsonArray>();

        for (auto p : c->getParameters())
        {

          JsonObject pObj = paramsArr.add<JsonObject>();
          pObj["name"] = p->getName();
          pObj["type"] = (int)p->getType();
          pObj["access"] = (int)p->getAccess();
        }
      }
    }
        serializeJsonPretty(doc, Serial);
    // Serialize to CBOR
    uint8_t configCBOR[512];
    size_t len = serializeMsgPack(doc, configCBOR, sizeof(configCBOR));

    return std::pair<uint8_t*, size_t>{configCBOR, len};
}

void YoloDevice::onBLENotify(NimBLEClient*, NimBLERemoteCharacteristic*)
{
  Serial.println("notify");
}