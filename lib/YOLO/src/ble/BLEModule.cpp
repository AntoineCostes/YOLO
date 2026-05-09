#include "BLEModule.h"

static BLEModule* gBLEModule = nullptr;

BLEModule::BLEModule() : Module<Component>("ble"),
                         // server
                         serverCbcks(nullptr), ctrlCbcks(),
                         // client
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
  }
  else
  {
    dbg("init server");
    serverCbcks = ServerCallbacks(isConnectedParam);
  }
}

void BLEModule::postInit(IYoloDeviceBLEContext &ctx)
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
  }
  else
  {
    dbg("post init server");
    // init service and characteristics
    // auto cbor = ctx.buildConfigCBOR();
    initService("BLEServer");
  }
}

void BLEModule::initService(const char *deviceName)
{
  // server only
  if (isClientParam->get())
    return;

  dbg("init service %s...", deviceName);

  NimBLEDevice::init("YOLO");
  NimBLEDevice::setMTU(247);
  server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCbcks);

  NimBLEService *service = server->createService(YOLO_SERVICE_UUID);

    queryChr = service->createCharacteristic(
    YOLO_QUERY_UUID,
    NIMBLE_PROPERTY::WRITE
    );

    responseChr = service->createCharacteristic(
    YOLO_RESPONSE_UUID,
    NIMBLE_PROPERTY::NOTIFY
);

  controlChr = service->createCharacteristic(
      YOLO_CONTROL_UUID,
      NIMBLE_PROPERTY::WRITE_NR);

  ctrlCbcks.setCallback([this](const uint8_t *data, size_t len)
                        {
            // Module emits higher-level event
            notifyControl(data, len); });
  controlChr->setCallbacks(&ctrlCbcks);

  stateChr = service->createCharacteristic(
      YOLO_STATE_UUID,
      NIMBLE_PROPERTY::NOTIFY);

  // configChr->setValue(configData, configDataSize);

  service->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName(deviceName);
  pAdvertising->addServiceUUID(service->getUUID());
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();
  dbg("advertising %s...", deviceName);
}

void BLEModule::handleQuery(uint8_t opcode,
                            const uint8_t* data,
                            size_t len)
{
    PacketWriter w;

    switch(opcode)
    {
        case GET_DEVICE_INFO:
        {
            w.u8(GET_DEVICE_INFO);
            w.u8(1); // protocol version
            // w.u8(modules.size());

            break;
        }

        case GET_MODULE_LIST:
        {
            w.u8(GET_MODULE_LIST);

            // w.u8(modules.size());

            // for(auto m : modules)
            // {
            //     w.u8(m->getModuleID());
            //     w.str(m->getName());
            // }

            break;
        }
    }

    responseChr->setValue(w.data.data(),
                          w.data.size());

    responseChr->notify();
}

void BLEModule::notify(Parameter *param)
{
  // server only
  if (isClientParam->get())
    return;

    PacketWriter w;

    w.u8(PARAM_NOTIFY);

    // w.u8(moduleId);
    // w.u8(componentId);
    // w.u8(paramId);

    w.u8((uint8_t)param->getType());

    param->serialize(w);

    stateChr->setValue(
        w.data.data(),
        w.data.size()
    );

    stateChr->notify();
}

void BLEModule::refresh()
{
  // server only
  if (!isClientParam->get())
  {
    if (isConnected())
    {
      // process server
    }
  } else
  {
    // client only
    for (auto &pair : clients)
    {

      switch (pair.second)
      {
      case ClientState::DISCONNECTED:
        log("DISCONNECTED");
        break;

      case ClientState::READY:
        break;

      case ClientState::DISCOVERING:
        log("DISCOVERING");
        break;

      case ClientState::CONNECTED:
        log("CONNECTED");

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
      
      if (ch->getUUID().toString() == YOLO_QUERY_UUID)
      {
      uint8_t cmd = GET_MODULE_LIST;
      ch->writeValue(&cmd, 1);
      }
      
      if (ch->getUUID().toString() == YOLO_RESPONSE_UUID)
      {
        ch->subscribe(
            true,
            BLEModule::onResponseStatic
        );
      }


      if (ch->getUUID().toString() == YOLO_STATE_UUID)
      {
        Serial.println();
        Serial.println("state found");
        // TODO ??
        if (ch->canNotify())
        {
          if (!ch->subscribe(true, BLEModule::onStateStatic)) {
        Serial.println("failed");
            pClient->disconnect();
            return false;
          }
        }
      }

    //   if (ch->getUUID().toString() == YOLO_CONTROL_UUID)
    //   {
    //     if (ch->writeValue("changed"))
    //     {
    //       Serial.printf("Wrote new value to: %s\n", ch->getUUID().toString().c_str());
    //     }
    //     else
    //     {
    //       pClient->disconnect();
    //       return false;
    //     }
    //   }
    //   Serial.println();
    }
  }
  log("subscribing done");
  return true;
}

void BLEModule::onResponse(
    uint8_t* data,
    size_t len)
{
    PacketReader r{data,0};

    uint8_t opcode = r.u8();

    switch(opcode)
    {
        case GET_MODULE_LIST:
        {
            uint8_t count = r.u8();

            for(int i=0;i<count;i++)
            {
                uint8_t id = r.u8();

                uint8_t len = r.u8();

                std::string name(
                    (char*)(data+r.pos),
                    len
                );

                r.pos += len;

                Serial.println(name.c_str());
            }

            break;
        }
    }
}