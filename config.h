#pragma once

// ============================================================
//  GhostWave ESP32 Firmware — Authorized Pentest Use Only
//  Target: LilyGO T-Embed CC1101 (ESP32-S3)
// ============================================================

// ── Build Info ───────────────────────────────────────────────
#define FW_NAME    "GhostWave"
#define FW_VERSION "2.0.0"
#define HW_TARGET  "T-Embed CC1101"

// ── Serial / USB ─────────────────────────────────────────────
// ESP32-S3 uses USB CDC — set Tools → USB CDC On Boot → Enabled
#define SERIAL_BAUD  115200

// ── T-Embed CC1101 Pin Definitions ───────────────────────────
// Verify against official schematic:
// https://github.com/Xinyuan-LilyGO/T-Embed-CC1101

// Status LED (WS2812 NeoPixel)
#define LED_PIN        8    // WS2812 data pin
#define LED_COUNT      1

// TFT Display (ST7789V2, 1.9" 170x320)
#define TFT_MOSI       35
#define TFT_SCLK       36
#define TFT_CS         34
#define TFT_DC         33
#define TFT_RST        38
#define TFT_BL         15   // Backlight PWM
#define TFT_WIDTH      170
#define TFT_HEIGHT     320

// Rotary Encoder
#define ENC_CLK        1
#define ENC_DT         2
#define ENC_SW         0    // Encoder push-button (active LOW)

// Battery voltage ADC
#define BAT_ADC_PIN    4
#define BAT_ADC_DIV    2.0f // Voltage divider ratio

// CC1101 Sub-GHz Radio (SPI2 / HSPI)
#define CC1101_MOSI    11
#define CC1101_MISO    13
#define CC1101_SCK     12
#define CC1101_CS      10
#define CC1101_GDO0    9    // IRQ / data
#define CC1101_GDO2    45   // Optional sync word detect

// SD Card (SPI3 / VSPI) — optional
#define SD_MOSI        40
#define SD_MISO        41
#define SD_SCK         39
#define SD_CS          42

// ── WiFi ─────────────────────────────────────────────────────
#define WIFI_SCAN_CHANNEL_MIN   1
#define WIFI_SCAN_CHANNEL_MAX   13
#define DEAUTH_REASON_CODE      0x0007
#define DEAUTH_BURST            10
#define BEACON_SPAM_COUNT       20
#define CHANNEL_HOP_INTERVAL_MS 200

// ── BLE (ESP32-S3 = BLE only, no Classic BT) ─────────────────
#define BLE_SCAN_DURATION_SEC   5
#define BLE_ADV_INTERVAL_MS     200

// ── CC1101 Sub-GHz ───────────────────────────────────────────
#define CC1101_DEFAULT_FREQ     433.92   // MHz
#define CC1101_DEFAULT_BW       200      // kHz  (58/100/200/325/400/540/650/800)
#define CC1101_DEFAULT_DRATE    4.8      // kBaud
#define CC1101_DEFAULT_DEVN     47.6     // kHz deviation
#define CC1101_RX_TIMEOUT_MS    5000
#define CC1101_MAX_RECORDINGS   16       // Max stored RF signals
#define CC1101_MAX_SIGNAL_LEN   512      // Max samples per recording

// Preset frequencies (MHz)
#define RF_PRESET_315           315.00
#define RF_PRESET_433           433.92
#define RF_PRESET_868           868.35
#define RF_PRESET_915           915.00

// ── Defense Thresholds ───────────────────────────────────────
#define DEAUTH_DETECT_THRESHOLD 5
#define DEAUTH_DETECT_WINDOW_MS 1000
