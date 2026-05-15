#include "display_manager.h"
#include "wifi_manager.h"
#include "bt_manager.h"
#include "cc1101_manager.h"

// ── Lifecycle ────────────────────────────────────────────────

void DisplayManager::begin() {
  // Backlight PWM
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
  ledcWrite(0, 200);

  tft.init();
  tft.setRotation(0);     // Portrait 170×320
  tft.fillScreen(C_BG);
  tft.setTextDatum(TL_DATUM);

  _pixel.begin();
  _pixel.setBrightness(40);
  ledOff();

  Serial.println("[Display] TFT initialized.");
}

void DisplayManager::setBrightness(uint8_t pct) {
  ledcWrite(0, map(pct, 0, 100, 0, 255));
}

// ── LED ───────────────────────────────────────────────────────

void DisplayManager::setLED(uint8_t r, uint8_t g, uint8_t b) {
  _pixel.setPixelColor(0, _pixel.Color(r, g, b));
  _pixel.show();
}

void DisplayManager::ledOff() {
  _pixel.setPixelColor(0, 0);
  _pixel.show();
}

void DisplayManager::ledPulse(uint8_t r, uint8_t g, uint8_t b, uint8_t steps) {
  for (uint8_t i = 0; i < steps; i++) {
    uint8_t br = i * 255 / steps;
    _pixel.setPixelColor(0, _pixel.Color(r * br / 255, g * br / 255, b * br / 255));
    _pixel.show(); delay(10);
  }
  for (int i = steps; i >= 0; i--) {
    uint8_t br = i * 255 / steps;
    _pixel.setPixelColor(0, _pixel.Color(r * br / 255, g * br / 255, b * br / 255));
    _pixel.show(); delay(10);
  }
  ledOff();
}

// ── Helpers ───────────────────────────────────────────────────

void DisplayManager::clearScreen() {
  tft.fillScreen(C_BG);
}

void DisplayManager::print(const char* text, uint16_t x, uint16_t y, uint8_t font, uint16_t color) {
  tft.setTextColor(color, C_BG);
  tft.drawString(text, x, y, font);
}

void DisplayManager::_drawTab(const char* label, uint8_t idx, uint8_t active,
                               uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  uint16_t bg  = (idx == active) ? C_ACCENT : C_DIM;
  uint16_t fg  = (idx == active) ? TFT_BLACK : TFT_WHITE;
  tft.fillRoundRect(x, y, w, h, 4, bg);
  tft.setTextColor(fg, bg);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(label, x + w/2, y + h/2, 1);
  tft.setTextDatum(TL_DATUM);
}

// ── Status Bar ────────────────────────────────────────────────

void DisplayManager::drawStatusBar(float battV, bool wifiActive, bool rfActive) {
  tft.fillRect(0, 0, TFT_WIDTH, 18, 0x2104); // dark grey strip

  // FW name
  tft.setTextColor(C_ACCENT, 0x2104);
  tft.drawString(FW_NAME, 4, 4, 1);

  // Icons
  char buf[24];
  snprintf(buf, sizeof(buf), "W:%s RF:%s", wifiActive ? "+" : "-", rfActive ? "+" : "-");
  tft.setTextColor(C_DIM, 0x2104);
  tft.drawString(buf, 70, 4, 1);

  // Battery
  uint8_t pct = constrain((battV - 3.2f) / 0.9f * 100.0f, 0, 100);
  uint16_t batColor = pct > 50 ? C_GREEN : (pct > 20 ? C_ORANGE : C_RED);
  snprintf(buf, sizeof(buf), "%.1fV", battV);
  tft.setTextColor(batColor, 0x2104);
  tft.drawString(buf, TFT_WIDTH - 32, 4, 1);
}

// ── Home Screen ───────────────────────────────────────────────

void DisplayManager::drawHome(Screen activeTab) {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT - 20, C_BG);

  // Big logo area
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_ACCENT, C_BG);
  tft.drawString("GhostWave", TFT_WIDTH/2, 60, 4);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("ESP32-S3 Pentest FW", TFT_WIDTH/2, 90, 1);
  tft.drawString("v" FW_VERSION, TFT_WIDTH/2, 102, 1);
  tft.setTextDatum(TL_DATUM);

  // Tab strip at bottom
  const char* tabs[] = {"WiFi", "BLE", "RF", "SYS"};
  uint16_t tw = TFT_WIDTH / 4;
  for (int i = 0; i < 4; i++) {
    _drawTab(tabs[i], i, (int)activeTab - 1, i * tw, TFT_HEIGHT - 28, tw - 2, 26);
  }
}

// ── WiFi Menu ─────────────────────────────────────────────────

void DisplayManager::drawWiFiMenu(uint8_t selected, bool sniffing, bool detecting) {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT - 48, C_BG);

  const char* items[] = {
    "Scan Networks",
    "AP List",
    "Promiscuous Sniff",
    "Deauth Attack",
    "Beacon Spam",
    "Probe Flood",
    "Evil Twin AP",
    "Deauth Detect"
  };
  const int N = 8;

  for (int i = 0; i < N; i++) {
    uint16_t y = 24 + i * 20;
    bool sel = (i == selected);
    if (sel) tft.fillRect(0, y - 2, TFT_WIDTH, 18, C_ACCENT);
    tft.setTextColor(sel ? TFT_BLACK : C_TEXT, sel ? C_ACCENT : C_BG);
    tft.drawString(items[i], 8, y, 2);
    // State badges
    if (i == 2 && sniffing)  { tft.setTextColor(C_GREEN, sel ? C_ACCENT : C_BG); tft.drawString("ON", 140, y, 2); }
    if (i == 7 && detecting) { tft.setTextColor(C_GREEN, sel ? C_ACCENT : C_BG); tft.drawString("ON", 140, y, 2); }
  }
}

// ── AP List ───────────────────────────────────────────────────

void DisplayManager::drawAPList(APRecord* nets, uint8_t count, uint8_t selected) {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT - 48, C_BG);

  if (count == 0) {
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("No APs. Run scan.", 8, 40, 2);
    return;
  }

  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("SSID         CH RSSI", 4, 22, 1);
  tft.drawFastHLine(0, 30, TFT_WIDTH, C_DIM);

  uint8_t visible = 12;
  uint8_t start   = selected >= visible ? selected - visible + 1 : 0;

  for (int i = start; i < min((int)count, (int)start + visible); i++) {
    uint16_t y   = 34 + (i - start) * 22;
    bool     sel = (i == selected);
    if (sel) tft.fillRect(0, y - 1, TFT_WIDTH, 20, C_ACCENT);

    uint16_t fg = sel ? TFT_BLACK : C_TEXT;
    tft.setTextColor(fg, sel ? C_ACCENT : C_BG);

    char ssid[15]; strncpy(ssid, nets[i].ssid, 14); ssid[14] = '\0';
    char row[32];
    snprintf(row, sizeof(row), "%-14s %2d %4d", ssid, nets[i].channel, nets[i].rssi);
    tft.drawString(row, 4, y, 1);
  }
}

// ── RF Menu ───────────────────────────────────────────────────

void DisplayManager::drawRFMenu(uint8_t selected, float freq, bool recording, bool jamming) {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT - 48, C_BG);

  // Frequency display
  char fbuf[24];
  snprintf(fbuf, sizeof(fbuf), "%.3f MHz", freq);
  tft.setTextColor(C_YELLOW, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(fbuf, TFT_WIDTH/2, 36, 4);
  tft.setTextDatum(TL_DATUM);

  if (jamming) {
    tft.setTextColor(C_RED, C_BG);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("!! JAMMING !!", TFT_WIDTH/2, 60, 2);
    tft.setTextDatum(TL_DATUM);
    return;
  }

  const char* items[] = {
    "Set Frequency",
    "Scan Spectrum",
    "Record Signal",
    "Replay Signal",
    "Brute Force",
    "Replay Detect",
    "Stored Signals",
    "Config"
  };
  const int N = 8;

  for (int i = 0; i < N; i++) {
    uint16_t y   = 72 + i * 20;
    bool     sel = (i == selected);
    if (sel) tft.fillRect(0, y - 2, TFT_WIDTH, 18, C_ACCENT);
    tft.setTextColor(sel ? TFT_BLACK : C_TEXT, sel ? C_ACCENT : C_BG);
    tft.drawString(items[i], 8, y, 2);
    if (i == 2 && recording) {
      tft.setTextColor(C_RED, sel ? C_ACCENT : C_BG);
      tft.drawString("REC", 132, y, 2);
    }
  }
}

// ── RF Signal List ────────────────────────────────────────────

void DisplayManager::drawRFSignalList(RFSignal* sigs, uint8_t count, uint8_t selected) {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT - 48, C_BG);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("# Freq(MHz) Bytes Label", 4, 22, 1);
  tft.drawFastHLine(0, 30, TFT_WIDTH, C_DIM);

  int shown = 0;
  for (int i = 0; i < CC1101_MAX_RECORDINGS; i++) {
    if (!sigs[i].valid) continue;
    uint16_t y   = 34 + shown * 22;
    bool     sel = (i == (int)selected);
    if (sel) tft.fillRect(0, y-1, TFT_WIDTH, 20, C_ACCENT);
    uint16_t fg = sel ? TFT_BLACK : C_TEXT;
    tft.setTextColor(fg, sel ? C_ACCENT : C_BG);
    char row[32];
    snprintf(row, sizeof(row), "%d %.3f %4d %s", i, sigs[i].frequency, sigs[i].len, sigs[i].label);
    tft.drawString(row, 4, y, 1);
    shown++;
  }
  if (shown == 0) {
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("No recordings yet.", 8, 50, 2);
  }
}

// ── RSSI Bar ──────────────────────────────────────────────────

void DisplayManager::drawRSSIBar(int8_t rssi, float freq) {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT - 48, C_BG);
  char fbuf[24];
  snprintf(fbuf, sizeof(fbuf), "%.3f MHz", freq);
  tft.setTextColor(C_YELLOW, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(fbuf, TFT_WIDTH/2, 30, 2);
  tft.setTextDatum(TL_DATUM);

  int8_t clamped = constrain(rssi, -120, -10);
  uint16_t barW  = map(clamped, -120, -10, 0, TFT_WIDTH - 20);
  uint16_t barColor = (rssi > -60) ? C_RED : (rssi > -90) ? C_ORANGE : C_GREEN;
  tft.fillRect(10, 60, barW, 20, barColor);
  tft.drawRect(10, 60, TFT_WIDTH - 20, 20, C_DIM);

  char rbuf[16];
  snprintf(rbuf, sizeof(rbuf), "%d dBm", rssi);
  tft.setTextColor(C_TEXT, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(rbuf, TFT_WIDTH/2, 92, 2);
  tft.setTextDatum(TL_DATUM);
}

// ── BLE List ──────────────────────────────────────────────────

void DisplayManager::drawBLEList(BLERecord* devs, uint8_t count, uint8_t selected) {
  tft.fillRect(0, 20, TFT_WIDTH, TFT_HEIGHT - 48, C_BG);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("Address           RSSI Name", 4, 22, 1);
  tft.drawFastHLine(0, 30, TFT_WIDTH, C_DIM);

  for (uint8_t i = 0; i < min((int)count, 12); i++) {
    uint16_t y   = 34 + i * 22;
    bool     sel = (i == selected);
    if (sel) tft.fillRect(0, y-1, TFT_WIDTH, 20, C_ACCENT);
    uint16_t fg = sel ? TFT_BLACK : C_TEXT;
    tft.setTextColor(fg, sel ? C_ACCENT : C_BG);
    char name[10]; strncpy(name, devs[i].name, 9); name[9] = '\0';
    char row[36];
    snprintf(row, sizeof(row), "%-17s %4d %-9s", devs[i].address, devs[i].rssi, name);
    tft.drawString(row, 4, y, 1);
  }
  if (count == 0) {
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("No BLE devices. Run blescan.", 8, 50, 1);
  }
}

// ── Message / Progress ────────────────────────────────────────

void DisplayManager::showMessage(const char* title, const char* msg, uint16_t color) {
  tft.fillRoundRect(10, 80, TFT_WIDTH - 20, 100, 8, 0x1082);
  tft.drawRoundRect(10, 80, TFT_WIDTH - 20, 100, 8, color);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(color, 0x1082);
  tft.drawString(title, TFT_WIDTH/2, 90, 2);
  tft.setTextColor(C_TEXT, 0x1082);
  tft.drawString(msg, TFT_WIDTH/2, 114, 1);
  tft.setTextDatum(TL_DATUM);
}

void DisplayManager::showProgress(const char* label, uint8_t pct) {
  tft.fillRect(10, 140, TFT_WIDTH - 20, 30, C_BG);
  tft.setTextColor(C_TEXT, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(label, TFT_WIDTH/2, 140, 1);
  tft.setTextDatum(TL_DATUM);
  uint16_t w = map(pct, 0, 100, 0, TFT_WIDTH - 20);
  tft.fillRect(10, 152, w, 12, C_ACCENT);
  tft.drawRect(10, 152, TFT_WIDTH - 20, 12, C_DIM);
}

DisplayManager disp;
