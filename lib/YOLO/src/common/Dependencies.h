#pragma once

#include <Arduino.h>

#include <vector>
#include <set>
#include <bitset>

// Files
#include <Preferences.h>
#include <LittleFS.h>
#include <FS.h>
#include <ArduinoJson.h>

// Servomotors
#include <ESP32Servo.h>

// Sensors
#include <Wire.h>

// BLE
#include <MD5Builder.h>  
#include <NimBLEDevice.h>
// NimBLEUUIDs format: xxxxxxxx-0000-4000-8000-xxxxxxxxxxxx
#define YOLO_SERVICE_UUID        "b91bb233-0000-4de7-97cb-ae80cc439000"
#define YOLO_QUERY_UUID         "b91bb233-0001-4de7-97cb-ae80cc439000"
#define YOLO_RESPONSE_UUID          "b91bb233-0002-4de7-97cb-ae80cc439000"
#define YOLO_STATE_UUID          "b91bb233-0003-4de7-97cb-ae80cc439000"
#define YOLO_CONTROL_UUID        "b91bb233-0004-4de7-97cb-ae80cc439000"