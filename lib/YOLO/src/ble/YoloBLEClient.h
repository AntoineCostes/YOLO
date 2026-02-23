#pragma once
#include "common/Component.h"
#include "common/Module.h"

enum ClientState
{
    CONNECTING,
    CONNECTED,
    DISCOVERING,
    READY,
    DISCONNECTED
};

class ClientCallbacks : public NimBLEClientCallbacks
{
public:
    ClientCallbacks(std::vector<std::pair<NimBLEClient *, ClientState>> *clients) : clients(clients) {}

    void onConnect(NimBLEClient *pClient) override
    {
        Serial.printf("Connected to: %s\n", pClient->getPeerAddress().toString().c_str());

        auto existingClient = std::find_if(clients->begin(), clients->end(),
                                           [pClient](const std::pair<NimBLEClient *, ClientState> &pair)
                                           { return pair.first == pClient; });
        if (existingClient != clients->end())
            existingClient->second = ClientState::CONNECTED;
    }

    void onDisconnect(NimBLEClient *pClient, int reason) override
    {
        Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);

        auto existingClient = std::find_if(clients->begin(), clients->end(),
                                           [pClient](const std::pair<NimBLEClient *, ClientState> &pair)
                                           { return pair.first == pClient; });

        if (existingClient != clients->end())
            existingClient->second = ClientState::DISCONNECTED;
    }

private:
    std::vector<std::pair<NimBLEClient *, ClientState>> *clients;
};

class ScanCallbacks : public NimBLEScanCallbacks
{
public:
    ScanCallbacks(BoolParameter *isScanningParam, std::vector<std::pair<NimBLEClient *, ClientState>> *clients, ClientCallbacks *clientCallbacks, int scanTimeMs) : isScanningParam(isScanningParam),
                                                                                                                                                                    clients(clients),
                                                                                                                                                                    clientCallbacks(clientCallbacks),
                                                                                                                                                                    scanTimeMs(scanTimeMs) {}
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
    {

        if (!advertisedDevice->isAdvertisingService(NimBLEUUID(YOLO_SERVICE_UUID)))
            return;

        Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());

        NimBLEDevice::getScan()->stop();

        auto pClient = NimBLEDevice::getDisconnectedClient();

        if (!pClient)
        {
            for (auto &pair : *clients)
                if (pair.first->getPeerAddress() == advertisedDevice->getAddress())
                {
                    Serial.println("Already managing this device");
                    return;
                }

            pClient = NimBLEDevice::createClient(advertisedDevice->getAddress());
            if (!pClient)
            {
                Serial.println("Client creation failed");
                return;
            }
            clients->push_back({pClient, ClientState::CONNECTING});
        }
        else
        {
            auto formerContext = std::find_if(clients->begin(), clients->end(),
                                              [pClient](const std::pair<NimBLEClient *, ClientState> &pair)
                                              { return pair.first == pClient; });

            if (formerContext == clients->end())
            {
                Serial.println("ERROR former client could not be found in list !");
                return;
            }
            Serial.println("former client was found");
        }

        pClient->setClientCallbacks(clientCallbacks, false);

        if (!pClient->connect(true, true, false))
        {
            NimBLEDevice::deleteClient(pClient);
            Serial.println("Connect failed");
            return;
        }
    }

    void onScanEnd(const NimBLEScanResults &results, int reason) override
    {
        Serial.printf("Scan Ended\n");
    }

private:
    BoolParameter *isScanningParam;
    std::vector<std::pair<NimBLEClient *, ClientState>> *clients;
    ClientCallbacks *clientCallbacks;
    int scanTimeMs;
};

class NotifyCallback : public NimBLECharacteristicCallbacks
{
public:
    void onStatus(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, int code) override
    {
        const std::string &val = pCharacteristic->getValue();
        Serial.println(String(val.c_str()));
        // call YoloDevice static callback
        // YoloDevice::instance().onBLEStateChangeStatic(nullptr,
        //    reinterpret_cast<const uint8_t*>(val.data()),
        //    val.size());
    }
};

// class YoloClient : public Component
// {
// public:
//     using NotifyCallback = void (*)(void *context, NimBLEClient*, NimBLERemoteCharacteristic* );

//     YoloClient(const char *name);
//     void refresh() override;

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
