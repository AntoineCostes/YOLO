#pragma once

#include <NimBLEDevice.h>

enum ConnectionState {
  CONNECTING,
  CONNECTED,
  DISCOVERING,
  READY,
  DISCONNECTED
};

enum BLEOpcode : uint8_t
{
  GET_MODULE_LIST = 0x01,
  GET_MODULE_DESC = 0x02,
  GET_COMPONENT_DESC = 0x03,
  GET_PARAM_VALUE = 0x04,
  SET_PARAM_VALUE = 0x05,
  PARAM_NOTIFY = 0x06
};

struct DeviceContext
{
    NimBLEClient* bleClient;

    ConnectionState state;

    NimBLERemoteCharacteristic* queryChr = nullptr;
    NimBLERemoteCharacteristic* responseChr = nullptr;
    NimBLERemoteCharacteristic* stateChr = nullptr;

    std::vector<RemoteModule> modules;

    DeviceContext(
        NimBLEClient* client,
        ConnectionState s)
        :
        bleClient(client),
        state(s)
    {
    }
};

// set matching device's state to CONNECTED or DISCONNECTED
class ClientCallbacks : public NimBLEClientCallbacks {
public:
  ClientCallbacks(std::vector<DeviceContext> *devices)
    : devices(devices) {}

  void onConnect(NimBLEClient *pClient) override {
    Serial.printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());

    auto existingDevice = std::find_if(devices->begin(), devices->end(),
                                       [pClient](const DeviceContext &device) {
                                         return device.bleClient == pClient;
                                       });
    if (existingDevice != devices->end())
      existingDevice->state = ConnectionState::CONNECTED;
  }

  void onDisconnect(NimBLEClient *pClient, int reason) override {
    Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);

    auto existingDevice = std::find_if(devices->begin(), devices->end(),
                                       [pClient](const DeviceContext &device) {
                                         return device.bleClient == pClient;
                                       });

    if (existingDevice != devices->end())
      existingDevice->state = ConnectionState::DISCONNECTED;
  }

private:
  std::vector<DeviceContext> *devices;
};

class ScanCallbacks : public NimBLEScanCallbacks {
public:
  ScanCallbacks(std::vector<DeviceContext> *devices, ClientCallbacks *clientCallbacks)
    : devices(devices), clientCallbacks(clientCallbacks){}
  void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {

    if (!advertisedDevice->isAdvertisingService(NimBLEUUID(YOLO_SERVICE_UUID)))
      return;

    Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());

    NimBLEDevice::getScan()->stop();

    auto pClient = NimBLEDevice::getDisconnectedClient();

    if (!pClient) {
      for (auto &device : *devices)
        if (device.bleClient->getPeerAddress() == advertisedDevice->getAddress()) {
          Serial.println("Already managing this device");
          return;
        }

      pClient = NimBLEDevice::createClient(advertisedDevice->getAddress());
      if (!pClient) {
        Serial.println("Client creation failed");
        return;
      }
      devices->emplace_back(pClient, ConnectionState::CONNECTING);
    } else {
      auto formerContext = std::find_if(devices->begin(), devices->end(),
                                        [pClient](const DeviceContext &device) {
                                          return device.bleClient == pClient;
                                        });

      if (formerContext == devices->end()) {
        Serial.println("ERROR former client could not be found in list !");
        return;
      }
      Serial.println("former client was found");
    }

    pClient->setClientCallbacks(clientCallbacks, false);

    if (!pClient->connect(true, true, false)) {
      NimBLEDevice::deleteClient(pClient);
      Serial.println("Connect failed");
      return;
    }
  }

  void onScanEnd(const NimBLEScanResults &results, int reason) override {
    Serial.printf("Scan Ended\n");
  }

private:
  std::vector<DeviceContext> *devices;
  ClientCallbacks *clientCallbacks;
};

void onResponseRaw(
  uint8_t *data,
  size_t len) {
  PacketReader r{ data, 0 };

  uint8_t opcode = r.u8();

  switch (opcode) {
    case GET_MODULE_LIST:
      {
        uint8_t count = r.u8();

        for (int i = 0; i < count; i++) {
          uint8_t id = r.u8();

          uint8_t nameLen = r.u8();

          std::string name(
            (char *)(data + r.pos),
            nameLen);

          r.pos += nameLen;

          Serial.println(name.c_str());
        }

        break;
      }
  }
}

void onResponse(
  NimBLERemoteCharacteristic *chr,
  uint8_t *data,
  size_t len,
  bool isNotify) {
  onResponseRaw(data, len);
}

void onStateRaw(
  uint8_t *data,
  size_t len) {
  // TODO
}

void onState(
  NimBLERemoteCharacteristic *chr,
  uint8_t *data,
  size_t len,
  bool isNotify) {
  onStateRaw(data, len);
}


class BLEProtocolClient {
public:

  bool connect(const std::string &address);

  bool discover(RemoteDevice &device);

  bool setParameter(
    uint8_t moduleId,
    uint8_t componentIndex,
    uint8_t paramIndex,
    const Variant &value);

  std::function<void(
    uint8_t,
    uint8_t,
    uint8_t,
    const Variant &)>
    onParameterChanged;
};
