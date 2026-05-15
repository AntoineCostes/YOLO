#pragma once
#include "BLEProtocol.h"

class BLEModule : public Module<MODULE_ID::BLE, Component>
{
public:
    BLEModule();

    void loadConfig(JsonObject const &config) override;
    virtual void setupServer(IYoloDeviceView &deviceView);

    // server
    void initService(const char *deviceName);
    void startAdvertising(const char *name);
    void notify(Parameter *param);
    bool isConnected()
    {
        if (isConnectedParam)
            return isConnectedParam->get();
        else
            return false;
    }

    void refresh() override;
    void initScan();
    void startScanning();

    void onControl(EventCallback<const uint8_t *, size_t> cb)
    {
        controlEvent = cb;
    }

    
void onResponse(
    uint8_t* data,
    size_t len);

protected:
    void notifyControl(const uint8_t *data, size_t len)
    {
        if (controlEvent)
            controlEvent(data, len);
    }
    EventCallback<const uint8_t *, size_t> controlEvent;

    int hack_inc;
    BoolParameter *isConnectedParam;

    NimBLEServer *server;
    ServerCallbacks serverCbcks;
    ControlCallbacks ctrlCbcks;

    NimBLECharacteristic *queryChr;    // discovery
    NimBLECharacteristic *responseChr; // answers
    QueryCallbacks* queryCallbacks = nullptr;

    NimBLECharacteristic *stateChr;    // update
    NimBLECharacteristic *controlChr;  // set param

    bool subscribe(NimBLEClient *pClient);
};
