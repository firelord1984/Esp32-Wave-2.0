<div align="center">

# 👻 GhostWave

### ESP32-S3 Penetration Testing Firmware
### for the LilyGO T-Embed CC1101

[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-blue?logo=espressif)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-teal?logo=arduino)](https://www.arduino.cc/)
[![Version](https://img.shields.io/badge/Version-2.0.0-brightgreen)]()
[![License](https://img.shields.io/badge/License-MIT-yellow)](https://github.com/firelord1984/Esp32-Wave-2.0/blob/main/LICENSE)
[![Authorized Use Only](https://img.shields.io/badge/⚠️_Use-Authorized_Testing_Only-red)]()

<br/>

*A modular, open-source wireless security research firmware combining*
*802.11 WiFi, Bluetooth LE, and sub-GHz RF (CC1101) red & blue team capabilities.*

<br/>

```
  ╔══════════════════════════════════════════════╗
  ║   GhostWave  v2.0  —  T-Embed CC1101         ║
  ║   ESP32-S3 + CC1101  Pentest Firmware        ║
  ║   FOR AUTHORIZED TESTING ONLY                ║
  ╚══════════════════════════════════════════════╝
```

</div>

---

## ⚠️ Legal Disclaimer

> This firmware is intended **exclusively** for authorized security research and penetration testing
> on networks and devices **you own** or have **explicit written permission** to test.
>
> Unauthorized use against third-party networks or devices is illegal under the
> **Computer Fraud and Abuse Act (US)**, **Computer Misuse Act (UK)**, and equivalent
> laws worldwide. RF jamming is illegal in virtually every jurisdiction outside a licensed,
> shielded test environment. The authors accept **no liability** for misuse.

---

## 📋 Table of Contents

- [Hardware](#-hardware)
- [Features Overview](#-features-overview)
- [WiFi Capabilities](#-wifi-capabilities)
- [Bluetooth BLE Capabilities](#-bluetooth-ble-capabilities)
- [Sub-GHz RF Capabilities](#-sub-ghz-rf-capabilities-cc1101)
- [Display & UI](#-display--ui)
- [Getting Started](#-getting-started)
- [Command Reference](#-command-reference)
- [Project Structure](#-project-structure)
- [Configuration](#-configuration)
- [Known Limitations](#-known-limitations)
- [Roadmap](#-roadmap)

---

## 🔧 Hardware

### LilyGO T-Embed CC1101

| Component | Spec |
|-----------|------|
| MCU | ESP32-S3 (Dual-core Xtensa LX7, 240 MHz) |
| Flash | 16 MB |
| PSRAM | 8 MB OPI |
| WiFi | 802.11 b/g/n 2.4 GHz |
| Bluetooth | BLE 5.0 (BLE only — no Classic BT on S3) |
| Sub-GHz Radio | CC1101 (300–928 MHz, OOK/FSK/GFSK/MSK) |
| Display | ST7789V2 1.9" TFT, 170×320 px |
| LED | WS2812 RGB NeoPixel |
| Input | Rotary encoder with push button |
| Power | LiPo battery with onboard charging |

### Pin Map

```
CC1101  →  MOSI:11  MISO:13  SCK:12  CS:10  GDO0:9   GDO2:45
TFT     →  MOSI:35  SCLK:36  CS:34   DC:33  RST:38   BL:15
Encoder →  CLK:1    DT:2     SW:0
NeoPixel→  DATA:8
Battery →  ADC:4
```

---

## ✨ Features Overview

```
GhostWave
├── 📡 WiFi (802.11)
│   ├── 🔴 Red Team
│   │   ├── Active Network Scanner
│   │   ├── Promiscuous Client Sniffer (channel hopping)
│   │   ├── Deauthentication Attack (targeted + broadcast)
│   │   ├── Beacon Frame Flood
│   │   ├── Probe Request Flood
│   │   └── Evil Twin / Rogue AP
│   └── 🔵 Blue Team
│       └── Deauth / Disassoc Storm Detector
│
├── 🦷 Bluetooth LE
│   ├── 🔴 Red Team
│   │   ├── Active BLE Scanner
│   │   ├── Custom Device Advertiser (spoof any peripheral)
│   │   ├── Apple Continuity Protocol Proximity Spam
│   │   ├── Google Fast Pair Popup Spam
│   │   ├── Samsung Fast Pair Popup Spam
│   │   └── GATT Server (logs connecting clients)
│   └── 🔵 Blue Team
│       └── Passive BLE Advertisement Monitor
│
└── 📻 Sub-GHz RF (CC1101, 300–928 MHz)
    ├── 🔴 Red Team
    │   ├── Raw Signal Recorder (16 slots)
    │   ├── Signal Replayer (replay attacks)
    │   ├── Frequency Sweep Scanner (RSSI bar chart)
    │   ├── De Bruijn Brute-Forcer (fixed-code systems)
    │   └── Continuous TX Jammer (lab/shielded use only)
    └── 🔵 Blue Team
        ├── Live RSSI Spectrum Monitor
        └── Replay Attack Detector
```

---

## 📡 WiFi Capabilities

### 🔴 Red Team

#### Network Scanner
Full active 802.11 scan returning a table of APs with BSSID, SSID, channel, RSSI, and encryption type (OPEN / WEP / WPA / WPA2 / WPA3).

#### Promiscuous Sniffer
Puts the radio into monitor mode and hops channels 1–13 every 200 ms (configurable). Extracts and logs:
- **Associated clients** extracted from data frame address fields
- **Probe requests** — logs the SSID being sought and requesting MAC

#### Deauthentication Attack
Crafts raw 802.11 deauthentication frames using `esp_wifi_80211_tx` and transmits them bidirectionally (AP→STA and STA→AP) to force disconnections. Supports broadcast (all clients on a BSS) or single targeted client MAC, with configurable burst rounds.

> **Note:** WPA3 networks with 802.11w PMF (Protected Management Frames) are immune.

#### Beacon Flood
Floods a channel with randomly generated beacon frames using locally-administered BSSIDs and common SSID prefixes, saturating the scan list of nearby devices.

#### Probe Request Flood
High-volume wildcard probe requests with randomized source MACs, forcing APs to respond and creating noise in RF monitoring systems.

#### Evil Twin / Rogue AP
Spins up a soft AP with any chosen SSID (open or WPA2). Combine with a captive portal for credential harvesting research in authorized engagements.

### 🔵 Blue Team

#### Deauth Detector
Monitors for deauthentication and disassociation management frames in promiscuous mode. Prints per-frame details (source, BSSID, reason code, RSSI) and raises a storm alert when the frame count exceeds a configurable threshold within a time window.

---

## 🦷 Bluetooth BLE Capabilities

> The ESP32-S3 supports **BLE only** — Classic Bluetooth is not available.

### 🔴 Red Team

#### BLE Scanner
Active scan reporting address, name, RSSI, appearance UUID, service UUIDs, and manufacturer data for all nearby peripherals.

#### Custom BLE Advertiser
Broadcasts the device as any named BLE peripheral with configurable appearance value for device impersonation research.

#### Apple Continuity Protocol Spam
Cycles through Apple Continuity protocol device type bytes (AirPods Pro/Max/Gen2/Gen3, AirDrop, iPhone Handoff) to generate iOS system notification popups. Based on publicly documented security research.

> iOS 17.2+ patched the most intrusive popup variants.

#### Google Fast Pair Spam
Broadcasts known model IDs via BLE service UUID `0xFE2C` to trigger Android device pairing dialogs. Publicly documented.

#### Samsung Fast Pair Spam
Sends Samsung manufacturer-specific advertisement payloads (company ID `0x0075`) with known Galaxy Buds/Watch device type bytes to trigger Galaxy wearable pairing popups on Android.

#### GATT Server
Live GATT server that logs all connecting BLE clients. Useful for testing BLE client enumeration and connection behavior.

### 🔵 Blue Team

#### Passive BLE Monitor
Continuous passive scan (no SCAN_REQ transmitted) that logs all advertising BLE devices without revealing the ESP32's presence.

---

## 📻 Sub-GHz RF Capabilities (CC1101)

The CC1101 covers **300–928 MHz** and supports OOK (ASK), 2-FSK, GFSK, 4-FSK, and MSK modulation. The full register-level driver is implemented from scratch with no external CC1101 library dependency.

### Common Frequency Presets

| Frequency | Region / Use |
|-----------|-------------|
| 315.00 MHz | US garage doors, older car remotes |
| 433.92 MHz | EU remotes, weather stations, IoT sensors |
| 868.35 MHz | EU LoRa/SigFox, smart meters |
| 915.00 MHz | US ISM band, some barrier/gate systems |

### 🔴 Red Team

#### Signal Recorder
Listens on a configured frequency and captures raw RF data into one of 16 named slots (up to 512 bytes each). Each recording stores the payload, frequency, modulation mode, and label.

#### Signal Replayer
Retransmits any stored recording at its original frequency, N times, with configurable inter-burst delay — the core **replay attack** capability for testing fixed-code systems.

#### Frequency Sweep Scanner
Sweeps a user-defined range with configurable step size, printing an ASCII RSSI bar chart to locate active transmitters:

```
  Freq(MHz)   RSSI(dBm)  Level
  433.420     -103       ###
  433.920      -52       ####################
  434.420      -89       #######
```

#### De Bruijn Brute-Forcer
Generates a de Bruijn sequence B(2, n) for a given bit width (4–12 bits) — a sequence that contains every possible n-bit pattern exactly once — and transmits it continuously against a target frequency. Effective against **fixed-code** RF remotes.

> Rolling-code systems (KeeLoq, HiTag2, AUT64) are **not** vulnerable to this technique.

#### Continuous TX Jammer
Puts the CC1101 into infinite TX mode with a full-carrier signal. **Use only in a shielded RF enclosure on your own equipment.** Illegal on public spectrum.

### 🔵 Blue Team

#### Live RSSI Monitor
Continuously samples and prints RSSI on the current frequency for real-time signal presence tracking.

#### Replay Attack Detector
Compares each incoming packet against the previous capture. Raises an alert when an identical packet is received more than once — the hallmark of a replay attack against a fixed-code system.

---

## 🖥️ Display & UI

The ST7789 1.9" TFT provides a live status UI navigated by the rotary encoder.

| Screen | Content |
|--------|---------|
| **Home** | Firmware name, version, tab bar |
| **WiFi** | Scrollable AP list or WiFi action menu |
| **BLE** | Scrollable BLE device list |
| **RF** | Current frequency, RSSI bar, recording/jam status |
| **Settings** | Redirects to serial console |

**Status bar** (permanent, top of display):
- Firmware name
- WiFi / RF active indicators
- Battery voltage with color-coded level (green → orange → red)

**NeoPixel LED status:**
- 🟢 Dim green — idle
- 🔵 Blue — booting
- 🔵 Cyan — blue team detector active
- 🔴 Red — jamming active

**Rotary Encoder:**
- Rotate — scroll menu / list selection
- Press — cycle between screens

---

## 🚀 Getting Started

### Prerequisites

- LilyGO T-Embed CC1101
- USB-C cable
- PlatformIO (recommended) or Arduino IDE 2.x

### Option A — PlatformIO (Recommended)

All TFT_eSPI build flags are pre-configured in `platformio.ini`. No manual library edits needed.

```bash
git clone https://github.com/yourusername/GhostWave.git
cd GhostWave
pio run -t upload
pio device monitor
```

### Option B — Arduino IDE 2.x

1. Add ESP32 board package via Boards Manager (Espressif, >= 2.0.14)
2. Install via Library Manager: `TFT_eSPI` by Bodmer, `Adafruit NeoPixel`
3. Edit `TFT_eSPI/User_Setup.h`:

```c
#define ST7789_DRIVER
#define TFT_WIDTH   170
#define TFT_HEIGHT  320
#define TFT_MOSI    35
#define TFT_SCLK    36
#define TFT_CS      34
#define TFT_DC      33
#define TFT_RST     38
#define TFT_BL      15
#define TFT_BACKLIGHT_ON HIGH
#define SPI_FREQUENCY  40000000
```

4. Board settings:

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| PSRAM | OPI PSRAM |
| Partition Scheme | Huge APP (3MB No OTA) |
| Flash Size | 16MB |
| Upload Speed | 921600 |

### First Boot

Connect serial at **115200 baud**:

```
  ╔══════════════════════════════════════════════╗
  ║   GhostWave  v2.0  —  T-Embed CC1101        ║
  ║   ESP32-S3 + CC1101  Pentest Firmware        ║
  ║   FOR AUTHORIZED TESTING ONLY                ║
  ╚══════════════════════════════════════════════╝

[*] Initializing display...
[*] Initializing WiFi...
[*] Initializing BLE...
[*] Initializing CC1101...
[CC1101] Part: 0x00  Version: 0x14
[CC1101] Ready at 433.920 MHz
[*] Ready.

> _
```

Type `help` for the full command list.

---

## 📖 Command Reference

### WiFi

```
scan                           Active scan, list nearby APs
list                           Re-print last scan results
sniff  /  sniff stop           Promiscuous sniffer (channel-hops 1–13)
detect  /  detect stop         Deauth/disassoc storm detector

deauth <idx> [rounds]          Broadcast deauth at AP by scan index
deauth <bssid> <mac> <ch>      Targeted deauth by MAC
beacon <ch> [count]            Beacon flood on channel (default 20)
probe  <ch> [count]            Probe request flood (default 50)
evil   <ssid> [pass] [ch]      Start evil twin AP
evil   stop                    Stop evil twin
```

### Bluetooth (BLE)

```
blescan [secs]                 Active BLE scan (default 5s)
blelist                        Print BLE scan results
blemon  /  blemon stop         Passive advertisement monitor

bleadv <name> [appearance]     Advertise as custom BLE device
bleadv stop                    Stop advertising
applespam                      Apple Continuity proximity popup research
gpspam                         Google Fast Pair popup research
sspam                          Samsung Fast Pair popup research
gatt  /  gatt stop             GATT server, logs connecting clients
```

### Sub-GHz RF (CC1101)

```
rf freq <mhz>                  Set frequency (300–928 MHz)
rf mod  <ook|2fsk|gfsk|4fsk|msk>    Set modulation
rf bw   <khz>                  Set bandwidth
rf dr   <kbaud>                Set data rate
rf pwr  <dbm>                  Set TX power (-30 to +10 dBm)
rf config                      Print current RF config
rf rssi                        Single RSSI reading

rf scan <start> <end> [step]   Sweep RSSI across frequency range
rf record <slot> [label]       Record signal to slot 0–15
rf replay <slot> [times]       Replay stored signal
rf replayall [times]           Replay all stored signals
rf signals                     List stored recordings
rf clear <slot>                Delete a recording slot
rf clearall                    Delete all recordings

rf brute <bits> [times]        De Bruijn brute-force (fixed-code remotes)
rf monitor  /  rf monitor stop Live RSSI spectrum monitor
rf detect   /  rf detect stop  Replay attack detector
rf jam <mhz>  /  rf jam stop   ⚠ Continuous TX jam (shielded lab only)
```

### System

```
info                           Chip info, MAC addresses, RF status, uptime
help / ?                       Full command reference
```

---

## 📁 Project Structure

```
GhostWave/
├── esp32_ghostwave_s3.ino     Main sketch — setup/loop, UI, battery ADC
├── config.h                   All pin definitions and tuneable constants
│
├── wifi_manager.h/.cpp        Scanner, sniffer, raw TX, attacks, detector
├── bt_manager.h/.cpp          BLE scan, advertise, spoof, GATT, monitor
├── cc1101_manager.h/.cpp      Full CC1101 driver: RX/TX, record/replay,
│                              sweep, brute-force, jam, spectrum monitor
│
├── display_manager.h/.cpp     ST7789 TFT screen layouts + WS2812 LED
├── encoder.h                  Interrupt-driven rotary encoder
├── menu.h/.cpp                Serial command parser — all subcommands
│
├── platformio.ini             PlatformIO build config (pre-configured)
└── README.md                  This file
```

---

## ⚙️ Configuration

All hardware pins and tuneable defaults are in `config.h`:

```c
// CC1101 sub-GHz defaults
#define CC1101_DEFAULT_FREQ     433.92  // MHz
#define CC1101_DEFAULT_BW       200     // kHz
#define CC1101_DEFAULT_DRATE    4.8     // kBaud
#define CC1101_MAX_RECORDINGS   16      // Signal slots
#define CC1101_MAX_SIGNAL_LEN   512     // Bytes per recording

// WiFi
#define DEAUTH_BURST            10      // Frames per burst
#define BEACON_SPAM_COUNT       20      // Fake APs per run
#define CHANNEL_HOP_INTERVAL_MS 200     // Sniffer channel hop speed (ms)

// Blue team
#define DEAUTH_DETECT_THRESHOLD 5       // Frames per window before alert
#define DEAUTH_DETECT_WINDOW_MS 1000
```

---

## ⚠️ Known Limitations

| Limitation | Detail |
|------------|--------|
| WPA3 immunity | Networks with 802.11w PMF are immune to deauth attacks |
| BLE-only | ESP32-S3 has no Classic Bluetooth — BLE only |
| Shared radio | BLE and WiFi share the 2.4 GHz radio; avoid running simultaneously |
| RAM-only recordings | RF signals are lost on reboot — SPIFFS persistence is on the roadmap |
| Fixed codes only | De Bruijn brute-force does not work against rolling-code systems (KeeLoq, HiTag2) |
| Apple patches | iOS 17.2+ patched the most intrusive proximity popup variants |
| Antenna range | The onboard trace antenna limits range — an external whip improves performance significantly |

---

## 🗺️ Roadmap

- [ ] SPIFFS/LittleFS signal persistence (save recordings across reboots)
- [ ] Captive portal for evil twin credential capture research
- [ ] Sub-GHz OOK pulse decoder (timing → binary visualization)
- [ ] PCAP-style WiFi frame logger to SD card
- [ ] Web UI over SoftAP for phone-based control
- [ ] OTA firmware updates

---

## 🙏 Acknowledgments

- [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder) — the original inspiration
- [LilyGO T-Embed CC1101](https://github.com/Xinyuan-LilyGO/T-Embed-CC1101) — hardware schematics and pinout
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) by Bodmer — display driver
- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32) by Espressif
- Security researchers who publicly documented BLE proximity advertisement protocols

---

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.

---

<div align="center">

**Built for security researchers. Use responsibly.**

</div>
