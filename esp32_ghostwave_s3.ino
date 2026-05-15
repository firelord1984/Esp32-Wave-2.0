/**
 *  GhostWave v2.0 — ESP32-S3 Pentest Firmware
 *  Target: LilyGO T-Embed CC1101
 *  =========================================
 *  LEGAL NOTICE: For authorized security research only.
 *  Only test networks and devices you own or have explicit
 *  written permission to test. The author accepts no
 *  liability for misuse.
 *
 *  Libraries needed (Arduino Library Manager):
 *    - TFT_eSPI          by Bodmer
 *    - Adafruit NeoPixel by Adafruit
 *    - ESP32 Arduino Core >= 2.0.14 (espressif/arduino-esp32)
 *
 *  Arduino IDE settings:
 *    Board      : ESP32S3 Dev Module
 *    USB Mode   : USB-OTG (TinyUSB)  or  USB CDC On Boot: Enabled
 *    Partition  : 16M Flash (3MB APP / 9.9MB FATFS)  or  Huge APP
 *    PSRAM      : OPI PSRAM
 *    Upload Speed: 921600
 *
 *  TFT_eSPI — edit User_Setup.h OR copy User_Setup_Select.h:
 *    #define ST7789_DRIVER
 *    #define TFT_WIDTH  170
 *    #define TFT_HEIGHT 320
 *    #define TFT_MOSI   35
 *    #define TFT_SCLK   36
 *    #define TFT_CS     34
 *    #define TFT_DC     33
 *    #define TFT_RST    38
 *    #define TFT_BL     15
 *    #define SPI_FREQUENCY  40000000
 *
 *  See platformio.ini for PlatformIO build (recommended —
 *  it sets all TFT_eSPI flags automatically via build_flags).
 */

#include "config.h"
#include "wifi_manager.h"
#include "bt_manager.h"
#include "cc1101_manager.h"
#include "display_manager.h"
#include "encoder.h"
#include "menu.h"

// ── Globals ───────────────────────────────────────────────────

Encoder enc;

// UI state
static Screen   currentScreen  = SCR_HOME;
static uint8_t  menuSelected   = 0;
static float    batteryVoltage = 4.1f;
static uint32_t lastBatCheck   = 0;
static uint32_t lastUIRefresh  = 0;

// ── Battery ADC ───────────────────────────────────────────────

static float readBattery() {
  int raw = analogRead(BAT_ADC_PIN);
  // ESP32-S3 ADC: 12-bit, 3.3V ref, resistor divider ×2
  return (raw / 4095.0f) * 3.3f * BAT_ADC_DIV;
}

// ── LED helpers ───────────────────────────────────────────────

static void ledStatus() {
  if (rfMgr.jamming)           disp.setLED(255, 0, 0);    // Red  = jamming
  else if (wifiMgr.deauthDetectorRunning()) disp.setLED(0, 200, 255); // Cyan = blue team
  else                          disp.setLED(0, 40, 0);    // Dim green = idle
}

// ── Encoder UI navigation ─────────────────────────────────────

static void handleEncoder() {
  int8_t delta = enc.getDelta();
  bool   press = enc.wasPressed();

  if (delta != 0) {
    menuSelected = (menuSelected + delta + 16) % 16;
    lastUIRefresh = 0;  // force immediate redraw
  }
  if (press) {
    // Advance screen on press
    currentScreen = (Screen)(((int)currentScreen + 1) % SCR_COUNT);
    menuSelected  = 0;
    lastUIRefresh = 0;
  }
}

// ── UI Refresh ────────────────────────────────────────────────

static void refreshUI() {
  if (millis() - lastUIRefresh < 250) return;
  lastUIRefresh = millis();

  disp.drawStatusBar(batteryVoltage,
    wifiMgr.deauthDetectorRunning() || wifiMgr.networkCount > 0,
    rfMgr.monitoring || rfMgr.jamming);

  switch (currentScreen) {
    case SCR_HOME:
      disp.drawHome(currentScreen);
      break;
    case SCR_WIFI:
      if (wifiMgr.networkCount > 0)
        disp.drawAPList(wifiMgr.networks, wifiMgr.networkCount, menuSelected);
      else
        disp.drawWiFiMenu(menuSelected,
          false /* sniffing flag — extend if needed */,
          wifiMgr.deauthDetectorRunning());
      break;
    case SCR_BT:
      disp.drawBLEList(btMgr.bleDevices, btMgr.bleDeviceCount, menuSelected);
      break;
    case SCR_RF:
      if (rfMgr.monitoring)
        disp.drawRSSIBar(rfMgr.readRSSI(), rfMgr.currentFreq);
      else
        disp.drawRFMenu(menuSelected, rfMgr.currentFreq,
          false, rfMgr.jamming);
      break;
    case SCR_SETTINGS:
      disp.showMessage("Settings", "Use serial console\nfor full control", TFT_CYAN);
      break;
    default: break;
  }
}

// ── Setup ─────────────────────────────────────────────────────

void setup() {
  // USB CDC serial — give host time to connect
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  printBanner();

  // Display + LED
  Serial.println("[*] Initializing display...");
  disp.begin();
  disp.setLED(0, 0, 80); // blue during boot

  disp.showMessage("GhostWave", "Booting...", TFT_CYAN);
  disp.drawStatusBar(4.1f, false, false);

  // Encoder
  enc.begin();

  // WiFi
  Serial.println("[*] Initializing WiFi...");
  disp.showProgress("WiFi", 20);
  wifiMgr.begin();

  // BLE
  Serial.println("[*] Initializing BLE...");
  disp.showProgress("BLE", 50);
  btMgr.begin();

  // CC1101
  Serial.println("[*] Initializing CC1101...");
  disp.showProgress("CC1101", 75);
  if (!rfMgr.begin()) {
    disp.showMessage("CC1101 ERROR", "Check SPI wiring!", TFT_RED);
    disp.setLED(255, 0, 0);
    delay(3000);
  } else {
    disp.showProgress("CC1101", 100);
  }

  delay(400);
  disp.clearScreen();

  Serial.println("[*] Ready.\n");
  printHelp();
  menuInit();

  disp.drawHome(SCR_HOME);
  disp.setLED(0, 40, 0);
}

// ── Loop ──────────────────────────────────────────────────────

void loop() {
  // Serial command processor
  menuTick();

  // Encoder navigation
  handleEncoder();

  // Background ticks
  wifiMgr.snifferTick();
  rfMgr.spectrumTick();
  rfMgr.replayDetectorTick();

  // Battery read every 10 s
  if (millis() - lastBatCheck > 10000) {
    batteryVoltage = readBattery();
    lastBatCheck   = millis();
  }

  // UI refresh (~4 Hz)
  refreshUI();

  // LED status heartbeat
  static uint32_t lastLed = 0;
  if (millis() - lastLed > 2000) {
    ledStatus();
    lastLed = millis();
  }
}
