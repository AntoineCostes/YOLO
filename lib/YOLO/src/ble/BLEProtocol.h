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

    virtual const std::vector<IModule*>&  getModules() const = 0;

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
            sendModuleList();
            break;
        default:
            break;
        }
    }

    void sendModuleList()
    {
        Serial.println("sendModuleList");
        const auto &modules = deviceView.getModules();
        std::vector<uint8_t> buffer;
        buffer.push_back(GET_MODULE_LIST);
        buffer.push_back(modules.size());
        for (auto m : modules)
        {
            buffer.push_back(m->getModuleID());
            const char *name = m->getName();
            uint8_t len = strlen(name);
            buffer.push_back(len);
            buffer.insert(buffer.end(), name, name + len);
        }
        responseChr->setValue(buffer.data(), buffer.size());
        responseChr->notify();
    }
};