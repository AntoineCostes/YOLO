#include "BLEModule.h"


BLEModule::BLEModule() : Module<Component>("ble"), 
//server
serverCbcks(nullptr), ctrlCbcks(),
//client
                                           clientCbcks(&clients),
                                           scanCbcks(nullptr, nullptr, nullptr, 1000),
                                           callback(nullptr),
                                           context(nullptr),
                                           hack_inc(0)

{
    initializedParam->set(false);

    isClientParam = new BoolParameter("isClient", ParamAccess::READ_ONLY, true);
    registerParam(isClientParam);

    isConnectedParam = new BoolParameter("isConnected", ParamAccess::READ_ONLY, false);
    registerParam(isConnectedParam);   

    // client
    isScanningParam = new BoolParameter("isScanning", ParamAccess::READ_ONLY, false);
    registerParam(isScanningParam);

    scanTimeParam = new IntParameter("scanTimeMs", ParamAccess::READ_ONLY, 5000, 1000, 10000);
    registerParam(scanTimeParam);
}

void BLEModule::loadConfig(JsonObject const &config)
{
    if (!config)
        return;
    Module::loadConfig(config);

    if (isClientParam->get())
    {
        dbg("init client");
        scanCbcks = ScanCallbacks(isScanningParam, &clients, &clientCbcks, scanTimeParam->get());

    } else
    {
        dbg("init server");
        serverCbcks = ServerCallbacks(isConnectedParam);
    }
}

void BLEModule::postInit(IYoloDeviceBLEContext& ctx)
{
    Serial.println("post init");
    
    if (isClientParam->get())
    {
        dbg("post init client");
        // init scan
        NimBLEDevice::init("Async-Client");
        NimBLEDevice::setPower(3); /** +3db */

        NimBLEScan *pScan = NimBLEDevice::getScan();
        pScan->setScanCallbacks(&scanCbcks);
        pScan->setInterval(12);
        pScan->setWindow(12);
        pScan->setActiveScan(true);
        pScan->start(scanTimeParam->get());

    } else 
    {
        dbg("post init server");
        // init service and characteristics
        auto cbor = ctx.buildConfigCBOR();
        initService("BLEServer", cbor.first, cbor.second);
    }
}

void BLEModule::initService(const char* deviceName, const uint8_t* configData, size_t configDataSize)
{
    // server only
    if (isClientParam->get())
        return;

    dbg("CBOR data size = %i bytes", configDataSize);
    dbg("init service %s...", deviceName);

    NimBLEDevice::init("YOLO");
    NimBLEDevice::setMTU(247);
    server = NimBLEDevice::createServer();
    server->setCallbacks(&serverCbcks);

    NimBLEService* service = server->createService(YOLO_SERVICE_UUID);
    controlChr = service->createCharacteristic(
        YOLO_CONTROL_UUID,
        NIMBLE_PROPERTY::WRITE_NR
    );

    ctrlCbcks.setCallback([this](const uint8_t* data, size_t len){
            // Module emits higher-level event
            notifyControl(data, len);
        });
    controlChr->setCallbacks(&ctrlCbcks);

    stateChr = service->createCharacteristic(
        YOLO_STATE_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );

    configChr = service->createCharacteristic(
        YOLO_CONFIG_UUID,
        NIMBLE_PROPERTY::READ
    );

    configChr->setValue(configData, configDataSize);

    service->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName(deviceName);
    pAdvertising->addServiceUUID(service->getUUID());
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();
    dbg("advertising %s...", deviceName);
}

void BLEModule::notify(Parameter* param)
{
    // server only
    if (isClientParam->get())
        return;

    log("notify");
    if (stateChr)
    {
    stateChr->setValue(param->toBytes(), param->getSize());
    stateChr->notify();

    }
}

void BLEModule::refresh()
{
    if (!isClientParam->get())
    { 
        hack_inc++;
        if (hack_inc == 10)
        {
          // dbg("ping");
          byte batteryVoltage = map(analogRead(0), 0, 4096, 0, 100);
          ByteParameter* param = new ByteParameter("battery", ParamAccess::READ_ONLY_ALWAYS_NOTIFY, batteryVoltage);
          stateChr->setValue(param->toBytes(), param->getSize());
          // Serial.printf("Sending parameter size = %d\n", param->getSize());
          stateChr->notify();
          hack_inc = 0;
        }
    } else 
          log("no client");
  return;

    // client only
  for (auto &pair : clients)
  {
          log(String(pair.second).c_str());

    switch (pair.second)
    {
    case ClientState::DISCONNECTED:
      break;

    case ClientState::READY:
      break;

    case ClientState::DISCOVERING:
      break;

    case ClientState::CONNECTED:

      if (pair.first->discoverAttributes())
      {
        pair.second = ClientState::READY;
        log("discovered !");
        subscribe(pair.first);
      }
      else
      {
        log("disconnected");
        pair.second = ClientState::DISCONNECTED;
      }
      break;
    }
  }
}


bool BLEModule::subscribe(NimBLEClient *pClient)
{
    // client only
    if (!isClientParam->get())
        return false;
        
    log("subscribing...");
  std::vector<NimBLERemoteService *> services = pClient->getServices(true);
  for (auto &svc : services)
  {

    Serial.printf("Service: %s\n", svc->getUUID().toString().c_str());

    std::vector<NimBLERemoteCharacteristic *> chars = svc->getCharacteristics(true);
    for (auto &ch : chars)
    {
      Serial.printf("  Char %s", ch->getUUID().toString().c_str());
      if (ch->canRead())
      {
        std::string value = ch->readValue();
        Serial.printf("  Value (%d bytes)= ", value.length());
        for (size_t i = 0; i < value.length(); i++)
        {
          Serial.printf("%02X ", (uint8_t)value[i]);
        }
      }
      if (ch->getUUID().toString() == YOLO_CONFIG_UUID)
      {
        Serial.println();
        Serial.println("config found");
        std::string raw = ch->readValue();
        Serial.printf("Config size: %d bytes\n", raw.length());

        const uint8_t *data = (const uint8_t *)raw.data();
        size_t size = raw.length();

        JsonDocument doc;
        DeserializationError err = deserializeMsgPack(doc, data, size);
        if (err)
        {
          Serial.print("CBOR decode failed: ");
          Serial.println(err.c_str());
          return false;
        }

        Serial.println("Config decoded:");
        serializeJson(doc, Serial);
        Serial.println();
      }
      if (ch->getUUID().toString() == YOLO_STATE_UUID)
      {
        Serial.println();
        Serial.println("state found");
        if (ch->canNotify())
        {
            // if (!ch->subscribe(true, BLEModule::gotNotification)) {
            //   pClient->disconnect();
            //   return false;
            // }
        }
      }

      if (ch->getUUID().toString() == YOLO_CONTROL_UUID)
      {
        if (ch->writeValue("changed"))
        {
          Serial.printf("Wrote new value to: %s\n", ch->getUUID().toString().c_str());
        }
        else
        {
          pClient->disconnect();
          return false;
        }
      }
      Serial.println();
    }
  }
        log("subscribing done");
  return true;
}