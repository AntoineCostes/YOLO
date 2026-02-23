#pragma once
#include "common/Dependencies.h"
#include "common/Module.h"
#include "FileManager.h"
#include "ble/BLEModule.h"
#include "servo/ServoModule.h"
#include "i2c/I2CModule.h"

class YoloDevice : public Component, IYoloDeviceBLEContext
{
public:
    static YoloDevice &instance()
    {
        static YoloDevice inst;
        return inst;
    }
    YoloDevice(const YoloDevice &) = delete;
    YoloDevice &operator=(const YoloDevice &) = delete;

    void init(String config = "");
    void update();
    void refresh() override {}

    std::pair<uint8_t*, size_t> buildConfigCBOR() override;
    void onBLENotify(NimBLEClient*, NimBLERemoteCharacteristic*)  override;
    std::string getDeviceName() const override {
        String name = FileManager::getCurrentConfigNiceName();
        std::string str(name.c_str(), name.length());
        return str;
    }


private:
    YoloDevice();

    std::vector<IModule *> modules;
    BLEModule *bleModule;

    void onParamChanged(Parameter *p);
    static void onParamChangedStatic(void *ctx, Parameter *p)
    {
        YoloDevice *self = static_cast<YoloDevice *>(ctx);
        self->onParamChanged(p);
    }

    // void handleBLEState(const uint8_t* data, size_t len);
    static void onBLEStateChangeStatic(void *ctx, NimBLEClient *pClient, NimBLERemoteCharacteristic* pChar) {
        // YoloDevice::instance().handleBLEState(data, len);
        Serial.println("state changed");
    }
};