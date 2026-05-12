#pragma once
#include "BLEProtocol.h"

class BLEModule : public Module<Component>
{
public:
    BLEModule();
    static constexpr uint8_t uuid = 0x00;
    uint8_t getModuleID() const override { return uuid; }

    void loadConfig(JsonObject const &config) override;
    virtual void setupServer(IYoloDeviceView &deviceView);

    void handleQuery(uint8_t opcode, const uint8_t *data, size_t len);

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
