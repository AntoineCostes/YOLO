#pragma once
#include "common/Component.h"
#include "common/Module.h"

class ServerCallbacks : public NimBLEServerCallbacks
{
public:
    ServerCallbacks(BoolParameter *isConnectedParam) : isConnectedParam(isConnectedParam) {}

    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo)
    {
        Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());
        pServer->updateConnParams(connInfo.getConnHandle(),
                                  12, // min interval
                                  24, // max interval
                                  0,  // latency
                                  60  // timeout
        );
        isConnectedParam->set(true);
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        Serial.printf("Client disconnected - start advertising\n");
        isConnectedParam->set(false);
        NimBLEDevice::startAdvertising();
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override
    {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
    }

private:
    BoolParameter *isConnectedParam;
};

class ControlCallbacks : public NimBLECharacteristicCallbacks
{
public:
    using Callback = std::function<void(const uint8_t *data, size_t len)>;

    ControlCallbacks() {}

    void onWrite(NimBLECharacteristic *chr,
                 NimBLEConnInfo &connInfo) override
    {
        if (callback)
        {
            auto val = chr->getValue();
            callback((uint8_t *)val.data(), val.size());
        }
    }

    void setCallback(Callback cb) { callback = cb; }

private:
    Callback callback;
};

// class YoloServer : public Component
// {
// public:
//     YoloServer(const char *name);
//     void refresh() override {}

//     void initService(const uint8_t* configData, size_t configDataSize);
//     void startAdvertising(const char *name);

//     void notify(Parameter *param);
//     bool isConnected()
//     {
//         if (isConnectedParam)
//             return isConnectedParam->get();
//         else
//             return false;
//     }

// protected:
//     BoolParameter *isConnectedParam;
//     NimBLEServer *server;
//     ServerCallbacks serverCbcks;
//     ControlCallbacks ctrlCbcks;

//     NimBLECharacteristic *controlChr;
//     NimBLECharacteristic *stateChr;
//     NimBLECharacteristic *configChr;
// };
