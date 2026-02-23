#include "BLEModule.h"


BLEModule::BLEModule() : Module<Component>("ble"), serverCbcks(nullptr), ctrlCbcks()
{
    initializedParam->set(false);

    isClientParam = new BoolParameter("isClient", ParamAccess::READ_ONLY, true);
    registerParam(isClientParam);

    isConnectedParam = new BoolParameter("isConnected", ParamAccess::READ_ONLY, false);
    registerParam(isConnectedParam);   
}

void BLEModule::loadConfig(JsonObject const &config)
{
    if (!config)
        return;
    Module::loadConfig(config);

    if (isClientParam->get())
    {
        dbg("init client");
    } else
    {
        dbg("init server");
        serverCbcks = ServerCallbacks(isConnectedParam);
    }
        // dbg("init client");
        // else
        // dbg("init server");

        // TODO scanTime as paramater ?
}

void BLEModule::postInit(IYoloDeviceBLEContext& ctx)
{
    Serial.println("post init");
    
    if (isClientParam->get())
    {
        dbg("post init client");

    } else 
    {
        dbg("post init server");
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
