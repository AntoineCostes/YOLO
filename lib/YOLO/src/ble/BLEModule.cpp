#include "BLEModule.h"

BLEModule::BLEModule() : Module<MODULE_ID::BLE, Component>("ble"),
                         serverCbcks(nullptr), ctrlCbcks(),
                         hack_inc(0)

{
  initializedParam->set(false);

  isConnectedParam = new BoolParameter("isConnected", ParameterAccess::READ_ONLY, false);
  registerParam(isConnectedParam);
}

void BLEModule::loadConfig(JsonObject const &config)
{
  if (!config)
    return;
  Module::loadConfig(config);

  dbg("init server");
  serverCbcks = ServerCallbacks(isConnectedParam);
}

void BLEModule::setupServer(IYoloDeviceView &deviceView)
{
  dbg("post init server");
  // init service and characteristics
  NimBLEDevice::init("YOLO");
  NimBLEDevice::setMTU(247);
  server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCbcks);

  NimBLEService *service = server->createService(YOLO_SERVICE_UUID);

  // client will send queries to retrieve architecture
  queryChr = service->createCharacteristic(YOLO_QUERY_UUID, NIMBLE_PROPERTY::WRITE);
  responseChr = service->createCharacteristic(YOLO_RESPONSE_UUID, NIMBLE_PROPERTY::NOTIFY);
  queryChr->setCallbacks(new QueryCallbacks(deviceView, responseChr));
  
  stateChr = service->createCharacteristic(YOLO_STATE_UUID, NIMBLE_PROPERTY::NOTIFY);
  controlChr = service->createCharacteristic(YOLO_CONTROL_UUID, NIMBLE_PROPERTY::WRITE_NR);

  ctrlCbcks.setCallback(
      [this](const uint8_t *data, size_t len)
      {
        // Module emits higher-level event
        notifyControl(data, len);
      });
  controlChr->setCallbacks(&ctrlCbcks);

  stateChr = service->createCharacteristic(
      YOLO_STATE_UUID,
      NIMBLE_PROPERTY::NOTIFY);

  service->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName(deviceView.getDeviceName());
  pAdvertising->addServiceUUID(service->getUUID());
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();
  dbg("advertising %s...", deviceView.getDeviceName());
}

void BLEModule::notify(Parameter *param)
{
  log("notify");
  if (stateChr)
  {
    stateChr->setValue(param->toBytes(), param->getSize());
    stateChr->notify();
  }
}

void BLEModule::refresh()
{

  hack_inc++;
  if (hack_inc == 100)
  {
    // dbg("ping");
    byte batteryVoltage = map(analogRead(0), 0, 4096, 0, 100);
    ByteParameter *param = new ByteParameter("battery", ParameterAccess::READ_ONLY_ALWAYS_NOTIFY, batteryVoltage);
    stateChr->setValue(param->toBytes(), param->getSize());
    // Serial.printf("Sending parameter size = %d\n", param->getSize());
    stateChr->notify();
    hack_inc = 0;
  }
}