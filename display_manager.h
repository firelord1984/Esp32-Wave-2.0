#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>       // Install: "TFT_eSPI" by Bodmer
#include <Adafruit_NeoPixel.h>
#include "config.h"

// ── TFT_eSPI User Setup (set in User_Setup.h or via build flags) ──
// If using platformio.ini, add:
//   build_flags =
//     -DUSER_SETUP_LOADED=1
//     -DST7789_DRIVER=1
//     -DTFT_WIDTH=170
//     -DTFT_HEIGHT=320
//     -DTFT_MOSI=35
//     -DTFT_SCLK=36
//     -DTFT_CS=34
//     -DTFT_DC=33
//     -DTFT_RST=38
//     -DTFT_BL=15
//     -DTFT_BACKLIGHT_ON=HIGH
//     -DLOAD_GLCD=1
//     -DLOAD_FONT2=1
//     -DLOAD_FONT4=1
//     -DSPI_FREQUENCY=40000000

// ── Colors ────────────────────────────────────────────────────
#define C_BG       TFT_BLACK
#define C_TEXT     TFT_WHITE
#define C_DIM      0x4228  // dark grey
#define C_ACCENT   0x07FF  // cyan
#define C_RED      TFT_RED
#define C_GREEN    0x07E0
#define C_ORANGE   0xFD20
#define C_YELLOW   TFT_YELLOW

// ── Screens ───────────────────────────────────────────────────
enum Screen {
  SCR_HOME = 0,
  SCR_WIFI,
  SCR_BT,
  SCR_RF,
  SCR_SETTINGS,
  SCR_COUNT
};

class DisplayManager {
public:
  void begin();
  void setBrightness(uint8_t pct);   // 0–100

  // ── Status bar (always drawn at top) ─────────────────────
  void drawStatusBar(float battV, bool wifiActive, bool rfActive);

  // ── Home screen ───────────────────────────────────────────
  void drawHome(Screen activeTab);

  // ── WiFi screens ──────────────────────────────────────────
  void drawWiFiMenu(uint8_t selected, bool sniffing, bool detecting);
  void drawAPList(struct APRecord* nets, uint8_t count, uint8_t selected);

  // ── RF screens ────────────────────────────────────────────
  void drawRFMenu(uint8_t selected, float freq, bool recording, bool jamming);
  void drawRFSignalList(struct RFSignal* sigs, uint8_t count, uint8_t selected);
  void drawRSSIBar(int8_t rssi, float freq);

  // ── BLE screen ────────────────────────────────────────────
  void drawBLEList(struct BLERecord* devs, uint8_t count, uint8_t selected);

  // ── Generic helpers ───────────────────────────────────────
  void showMessage(const char* title, const char* msg, uint16_t color = C_ACCENT);
  void showProgress(const char* label, uint8_t pct);
  void clearScreen();
  void print(const char* text, uint16_t x, uint16_t y,
             uint8_t font = 2, uint16_t color = C_TEXT);

  // ── NeoPixel LED ──────────────────────────────────────────
  void setLED(uint8_t r, uint8_t g, uint8_t b);
  void ledOff();
  void ledPulse(uint8_t r, uint8_t g, uint8_t b, uint8_t steps = 20);

  TFT_eSPI tft;

private:
  Adafruit_NeoPixel _pixel = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
  void _drawTab(const char* label, uint8_t idx, uint8_t active,
                uint16_t x, uint16_t y, uint16_t w, uint16_t h);
};

extern DisplayManager disp;
