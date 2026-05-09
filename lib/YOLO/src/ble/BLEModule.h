#pragma once
#include "common/Module.h"
#include "BLEServerCallbacks.h"
#include "BLEClientCallbacks.h"

struct BLEEvent
{
    BLEOpcode opcode;

    uint8_t moduleId;
    uint8_t componentId;
    uint8_t paramId;

    ParamType type;

    const uint8_t* payload;
    size_t payloadSize;
};

class IYoloDeviceBLEContext
{
public:
    virtual ~IYoloDeviceBLEContext() = default;

    // Used by BLEServerModule
    virtual std::pair<uint8_t *, size_t> buildConfigCBOR() = 0;

    // Used by BLEClientModule
    virtual void onBLENotify(NimBLEClient *,
                             NimBLERemoteCharacteristic *) = 0;

    virtual std::string getDeviceName() const = 0;
};

class BLEModule : public Module<Component>
{
public:
    BLEModule();
    static constexpr uint8_t uuid = 0x00;
    uint8_t getModuleID() const override { return uuid; }

    void loadConfig(JsonObject const &config) override;
    virtual void postInit(IYoloDeviceBLEContext &context);

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
    // Subscribe to control events
    // void onControl(EventCallback<const uint8_t*, size_t> cb) {
    //     controlCallback = cb;
    // }

    // void notifyControl(const uint8_t* data, size_t len) {
    //     if (controlCallback) controlCallback(data, len);
    // }

    // client
    using NotifyCallback = void (*)(void *context, NimBLEClient *, NimBLERemoteCharacteristic *);

    void refresh() override;
    void initScan();
    void startScanning();

    void setStateChangeCallback(NotifyCallback cb, void *ctx)
    {
        callback = cb;
        context = ctx;
    }

    void onControl(EventCallback<const uint8_t *, size_t> cb)
    {
        controlEvent = cb;
    }

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
    BoolParameter *isClientParam;
    BoolParameter *isConnectedParam;

    // server
    NimBLEServer *server;
    ServerCallbacks serverCbcks;
    ControlCallbacks ctrlCbcks;

    NimBLECharacteristic *queryChr;    // discovery
    NimBLECharacteristic *responseChr; // answers
    NimBLECharacteristic *stateChr;    // update
    NimBLECharacteristic *controlChr;  // set param

    // client
    BoolParameter *isScanningParam;
    IntParameter *scanTimeParam;
    std::vector<std::pair<NimBLEClient *, ClientState>> clients;
    ClientCallbacks clientCbcks;
    ScanCallbacks scanCbcks;

    bool subscribe(NimBLEClient *pClient);
    
    using EventCallback =
    std::function<void(const BLEEvent&)>;
    
    // NotifyCallback callback;
    // void *context;
    //     static void gotNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
};

class QueryCallbacks : public NimBLECharacteristicCallbacks
{
public:
    QueryCallbacks(BLEModule *module)
        : module(module) {}

    void onWrite(NimBLECharacteristic *chr,
                 NimBLEConnInfo &connInfo) override
    {
        auto val = chr->getValue();

        const uint8_t *data =
            reinterpret_cast<const uint8_t *>(val.data());

        uint8_t opcode = data[0];

        module->handleQuery(opcode, data, val.size());
    }

private:
    BLEModule *module;
};