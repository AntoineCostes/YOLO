#include "YoloClient.h"

NotifyCallback notifyCallbck;

YoloClient::YoloClient(const char *name) : Component(name, 100),
                                           clientCbcks(&clients),
                                           scanCbcks(nullptr, nullptr, nullptr, 1000),
                                           callback(nullptr),
                                           context(nullptr)
{
  initializedParam->set(true);

  isScanningParam = new BoolParameter("isScanning", ParamAccess::READ_ONLY, false);
  registerParam(isScanningParam);

  scanTimeParam = new IntParameter("scanTimeMs", ParamAccess::READ_ONLY, 5000, 1000, 10000);
  registerParam(scanTimeParam);

  scanCbcks = ScanCallbacks(isScanningParam, &clients, &clientCbcks, scanTimeParam->get());
}

void YoloClient::initScan()
{
  log("INIT SCAN");
  NimBLEDevice::init("Async-Client");
  NimBLEDevice::setPower(3); /** +3db */

  NimBLEScan *pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(&scanCbcks);
  pScan->setInterval(12);
  pScan->setWindow(12);
  pScan->setActiveScan(true);
  pScan->start(scanTimeParam->get());
}

void YoloClient::startScanning()
{
  log("start SCAN");
  NimBLEDevice::getScan()->start(scanTimeParam->get());
}

void YoloClient::refresh()
{
  for (auto &pair : clients)
  {

    switch (pair.second)
    {
    case ClientState::DISCONNECTED:
      break;

    case ClientState::READY:
      break;

    case ClientState::DISCOVERING:
      break;

    case ClientState::CONNECTED:

      if (pair.first->discoverAttributes())
      {
        pair.second = ClientState::READY;
        log("discovered !");
        subscribe(pair.first);
        // Serial.println(String(pair.first->toString().c_str()));
      }
      else
      {
        log("disconnected");
        pair.second = ClientState::DISCONNECTED;
      }
      break;
    }
  }
}

bool YoloClient::subscribe(NimBLEClient *pClient)
{
        log("subscribing...");
  std::vector<NimBLERemoteService *> services = pClient->getServices(true);
  for (auto &svc : services)
  {

    Serial.printf("Service: %s\n", svc->getUUID().toString().c_str());

    std::vector<NimBLERemoteCharacteristic *> chars = svc->getCharacteristics(true);
    for (auto &ch : chars)
    {
      Serial.printf("  Char %s", ch->getUUID().toString().c_str());
      if (ch->canRead())
      {
        std::string value = ch->readValue();
        Serial.printf("  Value (%d bytes)= ", value.length());
        for (size_t i = 0; i < value.length(); i++)
        {
          Serial.printf("%02X ", (uint8_t)value[i]);
        }
      }
      if (ch->getUUID().toString() == YOLO_CONFIG_UUID)
      {
        Serial.println();
        Serial.println("config found");
        std::string raw = ch->readValue();
        Serial.printf("Config size: %d bytes\n", raw.length());

        const uint8_t *data = (const uint8_t *)raw.data();
        size_t size = raw.length();

        JsonDocument doc;
        DeserializationError err = deserializeMsgPack(doc, data, size);
        if (err)
        {
          Serial.print("CBOR decode failed: ");
          Serial.println(err.c_str());
          return false;
        }

        Serial.println("Config decoded:");
        serializeJsonPretty(doc, Serial);
        Serial.println();
      }
      if (ch->getUUID().toString() == YOLO_STATE_UUID)
      {
        Serial.println();
        Serial.println("state found");
        if (ch->canNotify())
        {
            if (!ch->subscribe(true, YoloClient::gotNotification)) {
              pClient->disconnect();
              return false;
            }
        }
      }

      // if (ch->getUUID().toString() == YOLO_CONTROL_UUID)
      // {
      //   if (ch->writeValue("changed"))
      //   {
      //     Serial.printf("Wrote new value to: %s\n", ch->getUUID().toString().c_str());
      //   }
      //   else
      //   {
      //     pClient->disconnect();
      //     return false;
      //   }
      // }
      Serial.println();
    }
  }
        log("subscribing done");
  return true;
}