#include "BLEModule.h"

BLEModule::BLEModule() : Module<Component>("ble"),
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

  queryChr = service->createCharacteristic(YOLO_QUERY_UUID, NIMBLE_PROPERTY::WRITE);

  responseChr = service->createCharacteristic(YOLO_RESPONSE_UUID, NIMBLE_PROPERTY::NOTIFY);
  queryChr->setCallbacks( new QueryCallbacks(deviceView, responseChr));

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

void BLEModule::handleQuery(uint8_t opcode, const uint8_t *data, size_t len)
{
  log("handleQuery");
  Serial.println(opcode);
  switch ((BLEOpcode)opcode)
  {
  case BLEOpcode::GET_MODULE_LIST:
  {
    std::vector<uint8_t> packet;
    packet.push_back((uint8_t)BLEOpcode::GET_MODULE_LIST);
    packet.push_back(2);

    packet.push_back(0x01);
    packet.push_back(3);
    packet.push_back('b');
    packet.push_back('l');
    packet.push_back('e');

    packet.push_back(0x02);
    packet.push_back(4);
    packet.push_back('w');
    packet.push_back('i');
    packet.push_back('f');
    packet.push_back('i');

    responseChr->setValue(packet.data(), packet.size());
    responseChr->notify();

    break;
  }
  default:
    break;
  }
}
