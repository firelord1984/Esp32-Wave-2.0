#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>
#include "config.h"

// NOTE: ESP32-S3 supports BLE only (no Classic Bluetooth)

struct BLERecord {
  char    address[18];
  char    name[32];
  int8_t  rssi;
  uint16_t appearance;
  char    manufacturer[32];
  char    serviceUUID[40];
};

class BTManager {
public:
  void begin();
  void stop();

  // Scanning
  void bleScan(uint8_t durationSec = BLE_SCAN_DURATION_SEC);
  void printBLEDevices();

  // Advertising / Spoof
  void bleAdvertiseCustom(const char* name, uint16_t appearance = 0x0000);
  void bleSpamAppleProximity();
  void bleSpamGoogleFastPair();
  void bleSpamSamsungFastPair();
  void stopBLEAdvertise();

  // GATT server
  void startGATTServer();
  void stopGATTServer();

  // Blue team — log all BLE advertisements passively
  void startPassiveMonitor();
  void stopPassiveMonitor();

  BLERecord bleDevices[64];
  uint8_t   bleDeviceCount = 0;
  bool      monitoring     = false;

private:
  BLEScan*        _pScan       = nullptr;
  BLEServer*      _pServer     = nullptr;
  bool            _advertising = false;
};

extern BTManager btMgr;
