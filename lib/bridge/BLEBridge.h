#pragma once
#include <NimBLEDevice.h>


#define YOLO_SERVICE_UUID "b91bb233-0000-4de7-97cb-ae80cc439000"
#define YOLO_QUERY_UUID "b91bb233-0001-4de7-97cb-ae80cc439000"
#define YOLO_RESPONSE_UUID "b91bb233-0002-4de7-97cb-ae80cc439000"
#define YOLO_STATE_UUID "b91bb233-0003-4de7-97cb-ae80cc439000"
#define YOLO_CONTROL_UUID "b91bb233-0004-4de7-97cb-ae80cc439000"

enum class DiscoveryStep : uint8_t {
  Idle,
  Connect,
  Discover,
  BindCharacteristics,
  Subscribe,
  RequestModules,
  AwaitModules,
  RequestComponents,
  AwaitComponents,
  Ready,
  Error
};

enum class BLEOpcode : uint8_t {
  GET_DEVICE_INFO = 0x01,
  GET_MODULE_LIST = 0x02,
  GET_MODULE_DESC = 0x03,
  GET_COMPONENT_DESC = 0x04,
  GET_PARAM_VALUE = 0x05,
  SET_PARAM_VALUE = 0x06,
  PARAM_NOTIFY = 0x07
};

struct DeviceContext {
  NimBLEClient* bleClient = nullptr;

  DiscoveryStep step = DiscoveryStep::Idle;

  NimBLERemoteCharacteristic* queryChr = nullptr;
  NimBLERemoteCharacteristic* responseChr = nullptr;
  NimBLERemoteCharacteristic* stateChr = nullptr;

  std::vector<RemoteModule> modules;

  DeviceContext() {}

  DeviceContext(NimBLEClient* client)
    : bleClient(client) {}
};

class BLEBridge;

class ClientCallbacks : public NimBLEClientCallbacks {
public:
  ClientCallbacks(BLEBridge* owner)
    : owner(owner) {}
  void onConnect(NimBLEClient* pClient) override;
  void onDisconnect(NimBLEClient* pClient, int reason) override;
private:
  BLEBridge* owner;
};

class ScanCallbacks : public NimBLEScanCallbacks {
public:
  ScanCallbacks(BLEBridge* owner)
    : owner(owner) {}
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;
  void onScanEnd(const NimBLEScanResults& results, int reason) override;
private:
  BLEBridge* owner;
};

class BLEBridge {
public:
  BLEBridge()
    : clientCallbacks(this), scanCallbacks(this) {}

  void begin() {
    NimBLEDevice::init("YOLO-BRIDGE");
    NimBLEDevice::setPower(3);

    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(&scanCallbacks);
    scan->setInterval(45);
    scan->setWindow(15);
    scan->setActiveScan(true);
    scan->start(5000, false, true);
  }

  void update() {
    uint8_t opcode;
    std::vector<NimBLERemoteService*> services;

    for (auto& device : devices) {
      switch (device.step) {
        case DiscoveryStep::Discover:
          if (!device.bleClient->discoverAttributes()) {
            Serial.println("ERROR discover attributes failed");
            device.step = DiscoveryStep::Error;
            break;
          }

          device.step = DiscoveryStep::BindCharacteristics;
          break;

        case DiscoveryStep::BindCharacteristics:
          services = device.bleClient->getServices(true);
          for (auto svc : services) {
            auto chars = svc->getCharacteristics(true);
            for (auto ch : chars) {
              auto uuid = ch->getUUID().toString();

              if (uuid == YOLO_QUERY_UUID) device.queryChr = ch;
              else if (uuid == YOLO_RESPONSE_UUID) device.responseChr = ch;
              else if (uuid == YOLO_STATE_UUID) device.stateChr = ch;
            }
          }

          if (!device.queryChr || !device.responseChr || !device.stateChr) {
            Serial.println("ERROR Missing characteristics");
            device.step = DiscoveryStep::Error;
            break;
          }

          device.step = DiscoveryStep::Subscribe;
          break;

        case DiscoveryStep::Subscribe:
          device.responseChr->subscribe(true, [this](NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
            onResponse(chr, data, len, isNotify);
          });

          device.stateChr->subscribe(true, [this](NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
            onState(chr, data, len, isNotify);
          });

          device.step = DiscoveryStep::RequestModules;
          break;

        case DiscoveryStep::RequestModules:
          Serial.println("get modules");
          opcode = (uint8_t)BLEOpcode::GET_MODULE_LIST;
          device.queryChr->writeValue(&opcode, 1);

          device.step = DiscoveryStep::AwaitModules;
          break;

        case DiscoveryStep::AwaitModules:
          break;

        case DiscoveryStep::RequestComponents:
          opcode = (uint8_t)BLEOpcode::GET_COMPONENT_DESC;
          device.queryChr->writeValue(&opcode, 1);

          device.step = DiscoveryStep::AwaitComponents;
          break;

        default:
          break;
      }
    }
  }

  DeviceContext* findDevice(NimBLEClient* client) {
    for (auto& device : devices)
      if (device.bleClient == client)
        return &device;

    return nullptr;
  }

  void onConnected(NimBLEClient* client) {
    auto device = findDevice(client);
    if (!device) return;
    device->step = DiscoveryStep::Discover;
  }

  void onDisconnected(NimBLEClient* client) {
    auto device = findDevice(client);
    if (!device) return;
    device->step = DiscoveryStep::Idle;
  }

  void onResponse(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    Serial.println("on response");
    NimBLEClient* client = chr->getRemoteService()->getClient();
    auto device = findDevice(client);
    if (!device) {
      Serial.printf("eRROR client not found");
      return;
    }

    if (len == 0)
      return;
    uint8_t opcode = data[0];

    switch ((BLEOpcode)opcode) {
      case BLEOpcode::GET_MODULE_LIST:
        {
          static std::vector<uint8_t> snapshot;
          snapshot.assign(data, data + len);
          parseModuleList(*device, snapshot.data(), snapshot.size());
          break;
        }
      case BLEOpcode::GET_COMPONENT_DESC:
        {
          Serial.println("got components");
          break;
        }


      default:
        break;
    }
  }

  void onState(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    Serial.printf("STATE %d bytes\n", len);

    // TODO OSC forwarding
  }

private:
  std::vector<DeviceContext> devices;
  ClientCallbacks clientCallbacks;
  ScanCallbacks scanCallbacks;

  void startDiscovery(DeviceContext& device) {
    Serial.println("start discovery");
    uint8_t opcode = (uint8_t)BLEOpcode::GET_MODULE_LIST;
    device.queryChr->writeValue(&opcode, 1);
  }

  void parseModuleList(DeviceContext& device, uint8_t* data, size_t len) {
    Serial.println("parse module list");

    size_t pos = 1;
    uint8_t moduleCount = data[pos++];
    Serial.printf("MODULE COUNT = %d\n", moduleCount);

    for (int i = 0; i < moduleCount; i++) {
      Serial.printf("module #%d\n", i);
      RemoteModule mod;

      mod.id = data[pos++];
      Serial.printf("MODULE id = %d\n", mod.id);

      uint8_t paramCount = data[pos++];
      Serial.printf("param count = %d\n", paramCount);

      Serial.println("");
      Serial.println("list parameters");
      for (int paramIndex = 0; paramIndex < paramCount; paramIndex++) {

        Serial.printf("param #%d\n", paramIndex);

        uint8_t nameLen = data[pos++];
        Serial.printf("param nameLen = %d\n", nameLen);

        Serial.printf("RAW NAME BYTES: ");
        for (int i = 0; i < nameLen; i++)
        {
            Serial.printf("%02X ", data[pos + i]);
        }
        Serial.println();
        std::string paramName((char*)&data[pos], nameLen);
        Serial.println(paramName.c_str());
        Serial.printf("param name = %s\n", paramName.c_str());
        pos += nameLen;

        uint8_t paramType = data[pos++];
        Serial.printf("param type = %d\n", paramType);

        uint8_t paramAccess = data[pos++];
        Serial.printf("param access = %d\n", paramAccess);
      }

      uint8_t compCount = data[pos++];
      Serial.printf("comp count = %d\n", compCount);

      Serial.println("");
      Serial.println("list components");
      for (int compIndex = 0; compIndex < compCount; compIndex++) {

      Serial.printf("comp #%d\n", compIndex);
        uint8_t nameLen = data[pos++];
        std::string name((char*)&data[pos], nameLen);
        Serial.printf("comp name = %s\n", name.c_str());
        pos += nameLen;

        uint8_t compParamCount = data[pos++];
        Serial.printf("compParam count = %d\n", compParamCount);

        for (int i = 0; i < compParamCount; i++) {

          uint8_t compParamType = data[pos++];
          Serial.printf("compParam type = %d\n", compParamType);

          uint8_t compParamAccess = data[pos++];
          Serial.printf("compParam access = %d\n", compParamAccess);
        }
      }

      Serial.printf("MODULE %d : %s\n", mod.id, mod.name.c_str());
      device.modules.push_back(mod);
    }

    device.step = DiscoveryStep::RequestComponents;

    Serial.println("DISCOVERY DONE");
  }

public:
  friend class ClientCallbacks;
  friend class ScanCallbacks;
};


inline void ClientCallbacks::onConnect(NimBLEClient* pClient) {
  Serial.printf("Connected: %s\n", pClient->getPeerAddress().toString().c_str());
  owner->onConnected(pClient);
}

inline void ClientCallbacks::onDisconnect(
  NimBLEClient* pClient, int reason) {
  Serial.printf("Disconnected: %d\n", reason);
  owner->onDisconnected(pClient);
}

inline void ScanCallbacks::onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
  static NimBLEUUID yoloUUID(YOLO_SERVICE_UUID);

  if (!advertisedDevice->isAdvertisingService(yoloUUID)) {
    return;
  }

  Serial.printf("Found device: %s\n", advertisedDevice->toString().c_str());

  for (auto& device : owner->devices) {
    if (device.bleClient && device.bleClient->getPeerAddress() == advertisedDevice->getAddress()) {
      Serial.println("Already managed");
      return;
    }
  }

  auto client = NimBLEDevice::createClient(advertisedDevice->getAddress());
  if (!client) {
    Serial.println("Client creation failed");
    return;
  }
  owner->devices.emplace_back(client);
  client->setClientCallbacks(&owner->clientCallbacks, false);

  if (!client->connect(true, true, false)) {
    Serial.println("Connect failed");
    NimBLEDevice::deleteClient(client);
    return;
  }
}

inline void ScanCallbacks::onScanEnd(const NimBLEScanResults& results, int reason) {
  Serial.println("Scan ended");
}
