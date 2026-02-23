#include "YoloServer.h"


YoloServer::YoloServer(const char* name) : Component(name), serverCbcks(nullptr), ctrlCbcks()
{
    initializedParam->set(true);

    isConnectedParam = new BoolParameter("isConnected", ParamAccess::READ_ONLY, false);
    registerParam(isConnectedParam);
    
    serverCbcks = ServerCallbacks(isConnectedParam);
}


void YoloServer::initService(const uint8_t* configData, size_t configDataSize)
{
    log("data size %i", configDataSize);

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
    pAdvertising->setName("Default");
    pAdvertising->addServiceUUID(service->getUUID());
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();
    dbg("advertising...");

}

void YoloServer::notify(Parameter* param)
{
    log("notify");
    stateChr->setValue(param->toBytes(), param->getSize());
    stateChr->notify();
}