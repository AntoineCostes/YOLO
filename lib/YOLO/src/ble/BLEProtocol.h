#pragma once
#include "common/Module.h"

enum BLEOpcode : uint8_t
{
    GET_DEVICE_INFO = 0x01,
    GET_MODULE_LIST = 0x02,
    GET_MODULE_DESC = 0x03,
    GET_COMPONENT_DESC = 0x04,
    GET_PARAM_VALUE = 0x05,
    SET_PARAM_VALUE = 0x06,
    PARAM_NOTIFY = 0x07
};

// provide device info and architecture for BLE discovery
class IYoloDeviceView
{
public:
    virtual ~IYoloDeviceView() = default;

    virtual const std::vector<IModule *> &getModules() const = 0;

    virtual std::string getDeviceName() const = 0;
};

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

    void onWrite(NimBLECharacteristic *chr, NimBLEConnInfo &connInfo) override
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

// For BLE discovery
class QueryCallbacks : public NimBLECharacteristicCallbacks
{
public:
    QueryCallbacks(IYoloDeviceView &deviceView, NimBLECharacteristic *responseChr) : deviceView(deviceView), responseChr(responseChr) {}

    void onWrite(NimBLECharacteristic *chr, NimBLEConnInfo &connInfo) override
    {
        std::string value = chr->getValue();
        if (value.empty())
            return;
        uint8_t opcode = (uint8_t)value[0];
        handleOpcode(opcode);
    }

private:
    IYoloDeviceView &deviceView;
    NimBLECharacteristic *responseChr;

    void handleOpcode(uint8_t opcode)
    {
        Serial.println("handle op code");
        switch (opcode)
        {
        case GET_MODULE_LIST:
            getDeviceMap();
            break;

        default:
            break;
        }
    }

    void getDeviceMap()
    {
        Serial.println("sendDeviceMap");
        uint8_t buffer[1024];
        PacketWriter w(buffer, sizeof(buffer));

        w.u8((uint8_t)BLEOpcode::GET_MODULE_LIST);

        const auto& modules = deviceView.getModules();
        w.u8(modules.size());

        for (auto* m : modules)
            m->serializeMetadata(w);

        responseChr->setValue(buffer, w.pos);
        responseChr->notify();
    }
};