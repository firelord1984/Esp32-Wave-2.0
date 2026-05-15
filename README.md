# GhostWave v2.0 — T-Embed CC1101 (ESP32-S3)

> **⚠️ FOR AUTHORIZED USE ONLY** — Only test networks and devices you own
> or have explicit written permission to test. Unauthorized use is illegal.

---

## Hardware

**LilyGO T-Embed CC1101**
- ESP32-S3 (dual-core 240 MHz, BLE 5.0, 16 MB Flash, 8 MB PSRAM)
- ST7789V2 1.9" TFT 170×320
- CC1101 sub-GHz transceiver (300–928 MHz)
- WS2812 RGB LED
- Rotary encoder with push button
- LiPo battery connector + charging

---

## Build Options

### PlatformIO (Recommended)
```bash
pio run -t upload
```
All TFT_eSPI flags are pre-set in `platformio.ini`. No manual config needed.

### Arduino IDE 2.x
1. Install **ESP32 by Espressif** via Boards Manager (>= 2.0.14)
2. Install libraries via Library Manager:
   - `TFT_eSPI` by Bodmer
   - `Adafruit NeoPixel` by Adafruit
3. **Edit TFT_eSPI `User_Setup.h`** (in the library folder):
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
   - Board: **ESP32S3 Dev Module**
   - USB CDC On Boot: **Enabled**
   - PSRAM: **OPI PSRAM**
   - Partition Scheme: **Huge APP (3MB No OTA)**
   - Flash Size: **16MB**
5. Upload at 921600 baud

---

## Usage

Connect serial at **115200 baud**. Type `help` for the full command list.
The TFT display shows live status; rotate the encoder to navigate screens,
press to cycle between WiFi / BLE / RF / Settings views.

---

## Command Reference

### WiFi
| Command | Description |
|---------|-------------|
| `scan` | Active scan — lists APs with BSSID/channel/RSSI/encryption |
| `list` | Re-print last scan |
| `sniff` / `sniff stop` | Promiscuous sniffer, channel-hops 1–13, logs clients + probes |
| `detect` / `detect stop` | Deauth/disassoc storm detector (blue team) |
| `deauth <idx> [rounds]` | Broadcast deauth at AP from scan index |
| `deauth <bssid> <mac> <ch>` | Targeted deauth by MAC |
| `beacon <ch> [count]` | Beacon flood — fills channel with fake SSIDs |
| `probe <ch> [count]` | Probe request flood with rotating spoofed MACs |
| `evil <ssid> [pass] [ch]` | Evil twin / rogue AP |
| `evil stop` | Tear down rogue AP |

### Bluetooth (BLE — ESP32-S3 is BLE only)
| Command | Description |
|---------|-------------|
| `blescan [secs]` | Active BLE scan |
| `blelist` | Print BLE scan results |
| `blemon` / `blemon stop` | Passive advertisement monitor |
| `bleadv <name> [appear]` | Advertise as custom BLE device |
| `bleadv stop` | Stop advertising |
| `applespam` | Apple Continuity protocol proximity popup research |
| `gpspam` | Google Fast Pair popup research |
| `sspam` | Samsung Fast Pair popup research |
| `gatt` / `gatt stop` | GATT server — logs connecting clients |

### Sub-GHz RF (CC1101)
| Command | Description |
|---------|-------------|
| `rf freq <mhz>` | Set frequency (300–928 MHz) |
| `rf mod <ook\|2fsk\|gfsk\|4fsk\|msk>` | Set modulation |
| `rf bw <khz>` | Set bandwidth |
| `rf dr <kbaud>` | Set data rate |
| `rf pwr <dbm>` | Set TX power (−30 to +10 dBm) |
| `rf config` | Print current RF config |
| `rf rssi` | Single RSSI reading |
| `rf scan <start> <end> [step]` | Sweep frequencies, print RSSI bar chart |
| `rf record <slot> [label]` | Record signal into slot 0–15 |
| `rf replay <slot> [times]` | Replay stored signal |
| `rf replayall [times]` | Replay all stored signals |
| `rf signals` | List stored recordings |
| `rf clear <slot>` / `rf clearall` | Delete recording(s) |
| `rf brute <bits> [times]` | De Bruijn brute-force (fixed-code systems) |
| `rf monitor` / `rf monitor stop` | Live RSSI spectrum monitor on current freq |
| `rf detect` / `rf detect stop` | Replay attack detector |
| `rf jam <mhz>` / `rf jam stop` | ⚠ Continuous TX jam (shielded lab only) |

### System
| Command | Description |
|---------|-------------|
| `info` | Chip, MAC addresses, RF status, uptime |
| `help` / `?` | Full command list |

---

## Project Structure

```
esp32_ghostwave_s3/
├── esp32_ghostwave_s3.ino  ← Main sketch
├── config.h                ← All pin defs and tuneable constants
├── wifi_manager.h/.cpp     ← 802.11 scan, raw TX, sniffer, attacks, detector
├── bt_manager.h/.cpp       ← BLE scan, advertising, GATT, proximity spam
├── cc1101_manager.h/.cpp   ← Full CC1101 driver: RX/TX, record/replay, sweep, jam
├── display_manager.h/.cpp  ← ST7789 TFT + WS2812 LED
├── encoder.h               ← Interrupt-driven rotary encoder
├── menu.h/.cpp             ← Serial command parser
└── platformio.ini          ← PlatformIO build config (preferred)
```

---

## CC1101 Frequency Presets
| Band | Frequency | Use case |
|------|-----------|----------|
| 315 MHz | 315.00 | US garage doors, car remotes (older) |
| 433 MHz | 433.92 | EU remotes, weather stations, IoT sensors |
| 868 MHz | 868.35 | EU LoRa/SigFox, smart meters |
| 915 MHz | 915.00 | US ISM, some garage/barrier systems |

---

## Known Limitations
- **WPA3 with PMF** is immune to deauth attacks (802.11w)
- **BLE and WiFi share the radio** on ESP32-S3 — don't run both simultaneously
- **CC1101 max continuous TX power** at +10 dBm; follow local RF regulations
- **Rolling codes** (HiTag2, KeeLoq) cannot be brute-forced this way — `rf brute` only targets fixed-code systems
- **Apple proximity popups** patched in iOS 17.2+; Samsung/Google popups vary by OS version

---

## Legal Reminder
Use only on equipment you own or have written authorization to test.
RF jamming is illegal in virtually every jurisdiction outside a licensed,
shielded test environment.
