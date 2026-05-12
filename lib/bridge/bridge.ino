#include "Includes.h"
#include "BLEBridge.h"

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

Preferences preferences;

BLEBridge ble = BLEBridge();

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting YOLO bridge\n");
  
  ble.begin();

  // // INIT WIFI
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  // WiFi.begin(ssid, pass);
  // Serial.print("Searching for wifi network...");
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
  
  // Serial.println("MDNS...");
  // if (MDNS.begin("M5Stick_IMU"))
  // {
  //   MDNS.addService("_osc", "_udp", localPort);
  //   Serial.println("Done !");
  // }
  // else
  // {
  //   Serial.println("ERROR could not set up mDNS instance");
  // }
  
  // // INIT PREFERENCES
  // preferences.begin("network");
  // if (memorizeRemoteIP && preferences.isKey("remoteIp"))
  // {
  //   remoteIp.fromString(preferences.getString("remoteIp"));
  //   Serial.println("override remote IP with previous value: "+remoteIp.toString());
  // }
  // preferences.end();
  
  // // INIT OSC
  // Serial.println("Listening on " + String(localPort));
  // Udp.begin(localPort);
  // Udp.flush();
}

void loop() {
  ble.update();
}
