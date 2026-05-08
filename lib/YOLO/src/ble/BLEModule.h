#pragma once
#include "common/Module.h"
#include "YoloBLEServer.h"
#include "YoloBLEClient.h"

class IYoloDeviceBLEContext 
{
public:
    virtual ~IYoloDeviceBLEContext() = default;

    // Used by BLEServerModule
    virtual std::pair<uint8_t*, size_t> buildConfigCBOR() = 0;

    // Used by BLEClientModule
    virtual void onBLENotify(NimBLEClient*,
                             NimBLERemoteCharacteristic*) = 0;

    virtual std::string getDeviceName() const = 0;
};

class BLEModule : public Module<Component>
{
public:
  BLEModule();
  static constexpr uint8_t uuid = 0x00;
  uint8_t getModuleID() const override { return uuid; }
  
  void loadConfig(JsonObject const &config) override;
  virtual void postInit(IYoloDeviceBLEContext& context);

  // server
  void initService(const char* deviceName, const uint8_t* configData, size_t configDataSize);
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
  using NotifyCallback = void (*)(void *context, NimBLEClient*, NimBLERemoteCharacteristic* );

    void refresh() override;
    void initScan();
    void startScanning();

    void setStateChangeCallback(NotifyCallback cb, void *ctx)
    {
        callback = cb;
        context = ctx;
    }

    void onControl(EventCallback<const uint8_t*, size_t> cb) {
        controlEvent = cb;
    }

protected:
    void notifyControl(const uint8_t* data, size_t len) {
        if (controlEvent) controlEvent(data, len);
    }
    EventCallback<const uint8_t*, size_t> controlEvent;

    int hack_inc;
    BoolParameter *isClientParam;
    BoolParameter *isConnectedParam;

    // server
    NimBLEServer *server;
    ServerCallbacks serverCbcks;
    ControlCallbacks ctrlCbcks;

    NimBLECharacteristic *controlChr;
    NimBLECharacteristic *stateChr;
    NimBLECharacteristic *configChr;

    // client
    BoolParameter *isScanningParam;
    IntParameter *scanTimeParam;
    std::vector<std::pair<NimBLEClient *, ClientState>> clients;
    ClientCallbacks clientCbcks;
    ScanCallbacks scanCbcks;
    
    bool subscribe(NimBLEClient * pClient);
    NotifyCallback callback;
    void *context;

//     static void gotNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {


};

// class BLEServerModule : public BLEModule
// {
// public:
//   BLEServerModule();

//   void initService(const uint8_t* configData, size_t configDataSize);
//   void startAdvertising(const char *name);

//   void notify(Parameter *param);
//   bool isConnected()
//   {
//       if (isConnectedParam)
//           return isConnectedParam->get();
//       else
//           return false;
//   }


// protected:
//     NimBLEServer *server;
//     ServerCallbacks serverCbcks;
//     ControlCallbacks ctrlCbcks;

//     NimBLECharacteristic *controlChr;
//     NimBLECharacteristic *stateChr;
//     NimBLECharacteristic *configChr;
// };


// class BLEClientModule : public BLEServerModule
// {
// public:
//   using NotifyCallback = void (*)(void *context, NimBLEClient*, NimBLERemoteCharacteristic* );

//   BLEClientModule();
//   static constexpr uint8_t uuid = 0x01;
//   uint8_t getModuleID() const override { return uuid; }

//   void refresh() override {}

//     void initScan();
//     void startScanning();

//     void setStateChangeCallback(NotifyCallback cb, void *ctx)
//     {
//         callback = cb;
//         context = ctx;
//     }


// protected:
//     BoolParameter *isScanningParam;
//     IntParameter *scanTimeParam;
//     std::vector<std::pair<NimBLEClient *, ClientState>> clients;
//     ClientCallbacks clientCbcks;
//     ScanCallbacks scanCbcks;
    
//     bool subscribe(NimBLEClient * pClient);
//     NotifyCallback callback;
//     void *context;
    
//     static void gotNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
//     Serial.println("state changed");
//     // il faut pouvoir trigger le callback
// }
// };
