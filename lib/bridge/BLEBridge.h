#pragma once
#include <NimBLEDevice.h>

#define YOLO_SERVICE_UUID "b91bb233-0000-4de7-97cb-ae80cc439000"
#define YOLO_QUERY_UUID "b91bb233-0001-4de7-97cb-ae80cc439000"
#define YOLO_RESPONSE_UUID "b91bb233-0002-4de7-97cb-ae80cc439000"
#define YOLO_STATE_UUID "b91bb233-0003-4de7-97cb-ae80cc439000"
#define YOLO_CONTROL_UUID        "b91bb233-0004-4de7-97cb-ae80cc439000"

enum class ConnectionState : uint8_t {
  DISCONNECTED,
  CONNECTED,
  DISCOVERING,
  READY
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

  ConnectionState state = ConnectionState::DISCONNECTED;

  NimBLERemoteCharacteristic* queryChr = nullptr;
  NimBLERemoteCharacteristic* responseChr = nullptr;
  NimBLERemoteCharacteristic* stateChr = nullptr;

  std::vector<RemoteModule> modules;

  size_t discoveryIndex = 0;

  bool discoveryDone = false;

  DeviceContext() {}

  DeviceContext(NimBLEClient* client, ConnectionState s)
    : bleClient(client), state(s) {}
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
    for (auto& device : devices) {
      switch (device.state) {
        case ConnectionState::DISCONNECTED:
          break;

        case ConnectionState::CONNECTED:
          {
            Serial.println("CONNECTED");

            if (!device.bleClient->discoverAttributes()) {
              Serial.println("ERROR discover attributes failed");
              device.state = ConnectionState::DISCONNECTED;
              break;
            }

            if (!setupDevice(device)) {
              Serial.println("ERROR setupDevice failed");
              device.state = ConnectionState::DISCONNECTED;
              break;
            }
            startDiscovery(device);
            device.state = ConnectionState::DISCOVERING;
            break;
          }

        case ConnectionState::DISCOVERING:
          break;

        case ConnectionState::READY:
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
    device->state = ConnectionState::CONNECTED;
  }

  void onDisconnected(NimBLEClient* client) {
    auto device = findDevice(client);
    if (!device) return;
    device->state = ConnectionState::DISCONNECTED;
  }

  void onResponse(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    Serial.printf("on response");
    NimBLEClient* client = chr->getRemoteService()->getClient();
    auto device = findDevice(client);
    if (!device) 
    {
      
    Serial.printf("eRROR client not found");
      return;
    }
    parseResponse(*device, data, len);
  }

  void onState(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    Serial.printf("STATE %d bytes\n", len);

    // TODO OSC forwarding
  }

private:
  std::vector<DeviceContext> devices;
  ClientCallbacks clientCallbacks;
  ScanCallbacks scanCallbacks;


  bool setupDevice(DeviceContext& device) {
    auto services = device.bleClient->getServices(true);

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
      Serial.println("Missing characteristics");
      return false;
    }

    device.responseChr->subscribe(true, [this](NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
      onResponse(chr, data, len, isNotify);
    });

    device.stateChr->subscribe(true, [this](NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
      onState(chr, data, len, isNotify);
    });

    return true;
  }

  void startDiscovery(DeviceContext& device) {
    Serial.println("start discovery");
    uint8_t opcode = (uint8_t)BLEOpcode::GET_MODULE_LIST;
    device.queryChr->writeValue(&opcode, 1);
  }


  void parseResponse(DeviceContext& device, uint8_t* data, size_t len) {
    if (len == 0)
      return;

      Serial.println("parse response");
    uint8_t opcode = data[0];

    switch ((BLEOpcode)opcode) {
      case BLEOpcode::GET_MODULE_LIST:
        {
          parseModuleList(device, data, len);
          break;
        }

      default:
        break;
    }
  }

  void parseModuleList(DeviceContext& device, uint8_t* data, size_t len) {
    Serial.println("parse module list");

    size_t pos = 1;

    uint8_t count = data[pos++];

    Serial.printf("MODULE COUNT = %d\n", count);

    for (int i = 0; i < count; i++) {
      RemoteModule mod;

      mod.id = data[pos++];

      uint8_t nameLen = data[pos++];

      mod.name = std::string((char*)(data + pos), nameLen);

      pos += nameLen;

      Serial.printf("MODULE %d : %s\n", mod.id, mod.name.c_str());

      device.modules.push_back(mod);
    }

    device.discoveryDone = true;

    device.state = ConnectionState::READY;

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
  owner->devices.emplace_back(client, ConnectionState::DISCONNECTED);
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