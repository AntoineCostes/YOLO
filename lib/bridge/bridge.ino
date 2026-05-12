#include "Includes.h"
#include "BLEProtocol.h"

// dependencies: OSC by Adrian Freed, NimBLE-Arduino by h2zero
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

char ssid[] = "LeNet";                    // your network SSID (name)
char pass[] = "connectemoi";              // your network password

WiFiUDP Udp;
IPAddress remoteIp(192,168,17,232);       // target IP, will update if message is received
bool memorizeRemoteIP = true;             // if true, remoteIp will be erased with previous value
const unsigned int remotePort = 12000;    // target port, won't change
const unsigned int localPort = 12345;     // port receiving OSC

static constexpr uint32_t scanTimeMs = 5 * 1000;
std::vector<DeviceContext> *devices;
ClientCallbacks clientCbcks(devices);

Preferences preferences;

class OSCBridge
{
public:
    void attach(RemoteDevice* device);

    void onOSCMessage(
        const std::string& path,
        const Variant& value);

    void onBLEParameterChanged(
        uint8_t moduleId,
        uint8_t componentIndex,
        uint8_t paramIndex,
        const Variant& value);

private:

    RemoteDevice* device;
};

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting YOLO bridge\n");
  NimBLEDevice::init("YOLO bridge");
  NimBLEDevice::setPower(3); /** +3db */

  // INIT WIFI
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, pass);
  Serial.print("Searching for wifi network...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  Serial.println("MDNS...");
  if (MDNS.begin("M5Stick_IMU"))
  {
    MDNS.addService("_osc", "_udp", localPort);
    Serial.println("Done !");
  }
  else
  {
    Serial.println("ERROR could not set up mDNS instance");
  }
  
  // INIT PREFERENCES
  preferences.begin("network");
  if (memorizeRemoteIP && preferences.isKey("remoteIp"))
  {
    remoteIp.fromString(preferences.getString("remoteIp"));
    Serial.println("override remote IP with previous value: "+remoteIp.toString());
  }
  preferences.end();
  
  // INIT OSC
  Serial.println("Listening on " + String(localPort));
  Udp.begin(localPort);
  Udp.flush();

  // INIT BLE
  NimBLEScan *pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(new ScanCallbacks(devices, &clientCbcks));
  pScan->setInterval(12);
  pScan->setWindow(12);
  pScan->setActiveScan(true);
  pScan->start(scanTimeMs);
}

void loop() {
  // auto pClients = NimBLEDevice::getConnectedClients();

  for (auto &device : *devices) {

    switch (device.state) {
      case ConnectionState::DISCONNECTED:
        Serial.println("DISCONNECTED");
        break;

      case ConnectionState::DISCOVERING:
        Serial.println("DISCOVERING");
        break;

      case ConnectionState::CONNECTED:
        Serial.println("CONNECTED");
// discoverAttributes()
// récupérer les characteristics
// subscribe RESPONSE
// subscribe STATE
        // retrieve characteristics to subscribe to RESPONSE and STATE
        if (device.bleClient->discoverAttributes()) {
          device.state = ConnectionState::READY;
          Serial.println("discovered !");
          // subscribe(pair.first);
        } else {
          Serial.println("disconnected");
          device.state = ConnectionState::DISCONNECTED;
        }
        break;

      case ConnectionState::READY:
        break;
    }
  }
}


bool subscribe(NimBLEClient *pClient) {
  Serial.println("subscribing...");
  std::vector<NimBLERemoteService *> services = pClient->getServices(true);
  for (auto &svc : services) {

    Serial.printf("Service: %s\n", svc->getUUID().toString().c_str());

    std::vector<NimBLERemoteCharacteristic *> chars = svc->getCharacteristics(true);
    for (auto &ch : chars) {
      Serial.printf("  Char %s", ch->getUUID().toString().c_str());
      if (ch->canRead()) {
        std::string value = ch->readValue();
        Serial.printf("  Value (%d bytes)= ", value.length());
        for (size_t i = 0; i < value.length(); i++) {
          Serial.printf("%02X ", (uint8_t)value[i]);
        }
      }

      if (ch->getUUID().toString() == YOLO_QUERY_UUID) {
        uint8_t cmd = GET_MODULE_LIST;
        ch->writeValue(&cmd, 1);
      }

      if (ch->getUUID().toString() == YOLO_RESPONSE_UUID) {
        ch->subscribe(true, onResponse);
      }


      if (ch->getUUID().toString() == YOLO_STATE_UUID) {
        Serial.println();
        Serial.println("state found");
        if (ch->canNotify()) {
          if (!ch->subscribe(true, onState)) {
            Serial.println("failed");
            pClient->disconnect();
            return false;
          }
        }
      }
    }
  }
  Serial.println("subscribing done");
  return true;
}
