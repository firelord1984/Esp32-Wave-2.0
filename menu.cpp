#include "menu.h"
#include "wifi_manager.h"
#include "bt_manager.h"
#include "cc1101_manager.h"

static String _buf = "";

void printBanner() {
  Serial.println();
  Serial.println("  ╔══════════════════════════════════════════════╗");
  Serial.println("  ║   GhostWave  v" FW_VERSION "  —  " HW_TARGET "   ║");
  Serial.println("  ║   ESP32-S3 + CC1101  Pentest Firmware        ║");
  Serial.println("  ║   FOR AUTHORIZED TESTING ONLY                ║");
  Serial.println("  ╚══════════════════════════════════════════════╝");
  Serial.println();
}

void printHelp() {
  Serial.println("  ── WiFi ─────────────────────────────────────────────────────");
  Serial.println("  scan                     Active scan — list APs");
  Serial.println("  list                     Re-print last scan");
  Serial.println("  sniff  /  sniff stop     Promiscuous sniffer (ch-hop)");
  Serial.println("  detect  /  detect stop   Deauth/disassoc storm detector");
  Serial.println("  deauth <idx> [rounds]    Deauth broadcast at AP by scan index");
  Serial.println("  deauth <bssid> <mac> <ch>  Targeted deauth by MAC");
  Serial.println("  beacon <ch> [count]      Beacon flood on channel");
  Serial.println("  probe  <ch> [count]      Probe-request flood");
  Serial.println("  evil <ssid> [pass] [ch]  Evil twin AP");
  Serial.println("  evil stop");
  Serial.println();
  Serial.println("  ── Bluetooth (BLE) ──────────────────────────────────────────");
  Serial.println("  blescan [secs]           Active BLE scan");
  Serial.println("  blelist                  Print BLE results");
  Serial.println("  blemon  /  blemon stop   Passive BLE advertisement monitor");
  Serial.println("  bleadv <name> [appear]   Advertise as BLE device");
  Serial.println("  bleadv stop");
  Serial.println("  applespam                Apple Continuity proximity spam");
  Serial.println("  gpspam                   Google Fast Pair spam");
  Serial.println("  sspam                    Samsung Fast Pair spam");
  Serial.println("  gatt  /  gatt stop       GATT server (logs connections)");
  Serial.println();
  Serial.println("  ── Sub-GHz RF (CC1101) ──────────────────────────────────────");
  Serial.println("  rf freq <mhz>            Set frequency (e.g. rf freq 433.92)");
  Serial.println("  rf mod <ook|fsk|gfsk|msk>  Set modulation");
  Serial.println("  rf bw <khz>              Set bandwidth");
  Serial.println("  rf dr <kbaud>            Set data rate");
  Serial.println("  rf pwr <dbm>             Set TX power (-30 to +10)");
  Serial.println("  rf config                Print current RF config");
  Serial.println("  rf scan <start> <end> [step]  Sweep RSSI across freqs");
  Serial.println("  rf record <slot> [label] Record signal to slot 0-15");
  Serial.println("  rf replay <slot> [times] Replay stored signal");
  Serial.println("  rf replayall [times]     Replay all stored signals");
  Serial.println("  rf signals               List stored signals");
  Serial.println("  rf clear <slot>          Clear signal slot");
  Serial.println("  rf clearall              Clear all signal slots");
  Serial.println("  rf brute <bits> [times]  De Bruijn brute-force fixed codes");
  Serial.println("  rf monitor  /  rf monitor stop  RSSI spectrum monitor");
  Serial.println("  rf detect  /  rf detect stop    Replay attack detector");
  Serial.println("  rf jam <mhz>             !! ILLEGAL outside shielded lab !!  Jam freq");
  Serial.println("  rf jam stop");
  Serial.println("  rf rssi                  Single RSSI reading");
  Serial.println();
  Serial.println("  ── System ───────────────────────────────────────────────────");
  Serial.println("  info                     Device info / MAC addresses");
  Serial.println("  help / ?                 This menu");
  Serial.println();
}

// ── Tokeniser ────────────────────────────────────────────────

static bool parseMAC(const char* s, uint8_t* mac) {
  return sscanf(s, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
    &mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]) == 6;
}

static void handleCommand(const String& raw) {
  String line = raw; line.trim();
  if (!line.length()) return;

  const int MAX = 8;
  String args[MAX];
  int argc = 0, start = 0;
  for (int i = 0; i <= (int)line.length() && argc < MAX; i++) {
    if (i == (int)line.length() || line[i] == ' ') {
      if (i > start) args[argc++] = line.substring(start, i);
      start = i + 1;
    }
  }
  if (!argc) return;
  String cmd = args[0]; cmd.toLowerCase();

  // ── WiFi ─────────────────────────────────────────────────

  if (cmd == "scan") {
    wifiMgr.scanNetworks(); wifiMgr.printNetworks();

  } else if (cmd == "list") {
    wifiMgr.printNetworks();

  } else if (cmd == "sniff") {
    if (argc >= 2 && args[1] == "stop") wifiMgr.stopPromiscuousSniffer();
    else                                 wifiMgr.startPromiscuousSniffer();

  } else if (cmd == "detect") {
    if (argc >= 2 && args[1] == "stop") {
      wifiMgr.stopDeauthDetector();
      Serial.printf("[BLUE TEAM] %d deauth/disassoc frames captured.\n", wifiMgr.deauthFrameCount());
    } else { wifiMgr.startDeauthDetector(); }

  } else if (cmd == "deauth") {
    if (argc < 2) { Serial.println("Usage: deauth <idx> [rounds] | deauth <bssid> <mac> <ch>"); return; }
    uint8_t apBSSID[6], tgtMAC[6];
    if (parseMAC(args[1].c_str(), apBSSID)) {
      if (argc < 4 || !parseMAC(args[2].c_str(), tgtMAC)) {
        Serial.println("Usage: deauth <ap_bssid> <client_mac> <channel>"); return; }
      wifiMgr.deauthAttack(apBSSID, tgtMAC, args[3].toInt());
    } else {
      int idx = args[1].toInt();
      if (idx < 0 || idx >= wifiMgr.networkCount) {
        Serial.printf("Bad index (0–%d). Run scan first.\n", wifiMgr.networkCount-1); return; }
      memcpy(apBSSID, wifiMgr.networks[idx].bssid, 6);
      uint16_t rounds = argc >= 3 ? args[2].toInt() : 0;
      wifiMgr.deauthAttack(apBSSID, nullptr, wifiMgr.networks[idx].channel, rounds);
    }

  } else if (cmd == "beacon") {
    uint8_t ch   = argc >= 2 ? args[1].toInt() : 6;
    uint16_t cnt = argc >= 3 ? args[2].toInt() : BEACON_SPAM_COUNT;
    if (ch < 1 || ch > 13) { Serial.println("Channel 1-13 only."); return; }
    wifiMgr.beaconSpam(ch, cnt);

  } else if (cmd == "probe") {
    uint8_t  ch  = argc >= 2 ? args[1].toInt() : 6;
    uint16_t cnt = argc >= 3 ? args[2].toInt() : 50;
    if (ch < 1 || ch > 13) { Serial.println("Channel 1-13 only."); return; }
    wifiMgr.probeFlood(ch, cnt);

  } else if (cmd == "evil") {
    if (argc >= 2 && args[1] == "stop") wifiMgr.stopEvilTwin();
    else if (argc >= 2) {
      const char* pass = argc >= 3 ? args[2].c_str() : nullptr;
      uint8_t ch       = argc >= 4 ? args[3].toInt() : 1;
      wifiMgr.evilTwin(args[1].c_str(), pass, ch);
    } else { Serial.println("Usage: evil <ssid> [pass] [ch]  |  evil stop"); }

  // ── BLE ──────────────────────────────────────────────────

  } else if (cmd == "blescan") {
    btMgr.bleScan(argc >= 2 ? args[1].toInt() : BLE_SCAN_DURATION_SEC);

  } else if (cmd == "blelist") {
    btMgr.printBLEDevices();

  } else if (cmd == "blemon") {
    if (argc >= 2 && args[1] == "stop") btMgr.stopPassiveMonitor();
    else                                 btMgr.startPassiveMonitor();

  } else if (cmd == "bleadv") {
    if (argc >= 2 && args[1] == "stop") btMgr.stopBLEAdvertise();
    else if (argc >= 2) {
      uint16_t appear = argc >= 3 ? strtol(args[2].c_str(), nullptr, 0) : 0;
      btMgr.bleAdvertiseCustom(args[1].c_str(), appear);
    } else { Serial.println("Usage: bleadv <name> [appearance_hex]  |  bleadv stop"); }

  } else if (cmd == "applespam")  { btMgr.bleSpamAppleProximity();
  } else if (cmd == "gpspam")     { btMgr.bleSpamGoogleFastPair();
  } else if (cmd == "sspam")      { btMgr.bleSpamSamsungFastPair();

  } else if (cmd == "gatt") {
    if (argc >= 2 && args[1] == "stop") btMgr.stopGATTServer();
    else                                 btMgr.startGATTServer();

  // ── RF ───────────────────────────────────────────────────

  } else if (cmd == "rf") {
    if (argc < 2) { Serial.println("Usage: rf <subcommand>  — type 'help' for list."); return; }
    String sub = args[1]; sub.toLowerCase();

    if (sub == "freq") {
      if (argc < 3) { Serial.println("Usage: rf freq <mhz>"); return; }
      float f = args[2].toFloat();
      if (f < 300.0f || f > 928.0f) { Serial.println("CC1101 range: 300-928 MHz."); return; }
      rfMgr.setFrequency(f);
      Serial.printf("[RF] Frequency set to %.3f MHz\n", f);

    } else if (sub == "mod") {
      if (argc < 3) { Serial.println("Usage: rf mod <ook|2fsk|gfsk|4fsk|msk>"); return; }
      String m = args[2]; m.toLowerCase();
      if      (m == "ook" || m == "ask") rfMgr.setModulation(MOD_ASK);
      else if (m == "2fsk")              rfMgr.setModulation(MOD_2FSK);
      else if (m == "gfsk")              rfMgr.setModulation(MOD_GFSK);
      else if (m == "4fsk")              rfMgr.setModulation(MOD_4FSK);
      else if (m == "msk")               rfMgr.setModulation(MOD_MSK);
      else { Serial.println("Modulations: ook, 2fsk, gfsk, 4fsk, msk"); return; }
      Serial.printf("[RF] Modulation set to %s\n", args[2].c_str());

    } else if (sub == "bw") {
      if (argc < 3) { Serial.println("Usage: rf bw <khz>"); return; }
      rfMgr.setBandwidth(args[2].toInt());
      Serial.printf("[RF] Bandwidth ~%d kHz\n", args[2].toInt());

    } else if (sub == "dr") {
      if (argc < 3) { Serial.println("Usage: rf dr <kbaud>"); return; }
      rfMgr.setDataRate(args[2].toFloat());
      Serial.printf("[RF] Data rate ~%.2f kBaud\n", args[2].toFloat());

    } else if (sub == "pwr") {
      if (argc < 3) { Serial.println("Usage: rf pwr <dbm>  (-30 to +10)"); return; }
      rfMgr.setOutputPower(args[2].toInt());
      Serial.printf("[RF] TX power ~%d dBm\n", args[2].toInt());

    } else if (sub == "config") {
      rfMgr.printConfig();

    } else if (sub == "rssi") {
      // startSpectrumMonitor puts chip in RX; read then stop
      rfMgr.startSpectrumMonitor();
      delay(10);
      Serial.printf("[RF] RSSI: %d dBm  LQI: %d\n", rfMgr.readRSSI(), rfMgr.readLQI());
      rfMgr.stopSpectrumMonitor();

    } else if (sub == "scan") {
      if (argc < 4) { Serial.println("Usage: rf scan <start_mhz> <end_mhz> [step_mhz]"); return; }
      float step = argc >= 5 ? args[4].toFloat() : 0.5f;
      rfMgr.scanFrequencies(args[2].toFloat(), args[3].toFloat(), step);

    } else if (sub == "record") {
      if (argc < 3) { Serial.println("Usage: rf record <slot 0-15> [label]"); return; }
      int slot = args[2].toInt();
      const char* label = argc >= 4 ? args[3].c_str() : nullptr;
      rfMgr.recordSignal(slot, label);

    } else if (sub == "replay") {
      if (argc < 3) { Serial.println("Usage: rf replay <slot> [times]"); return; }
      rfMgr.replaySignal(args[2].toInt(), argc >= 4 ? args[3].toInt() : 1);

    } else if (sub == "replayall") {
      rfMgr.replayAll(argc >= 3 ? args[2].toInt() : 1);

    } else if (sub == "signals") {
      rfMgr.printSignals();

    } else if (sub == "clear") {
      if (argc < 3) { Serial.println("Usage: rf clear <slot>"); return; }
      rfMgr.clearSignal(args[2].toInt());

    } else if (sub == "clearall") {
      rfMgr.clearAllSignals();
      Serial.println("[RF] All slots cleared.");

    } else if (sub == "brute") {
      if (argc < 3) { Serial.println("Usage: rf brute <bits 4-12> [times]"); return; }
      rfMgr.bruteForce(args[2].toInt(), argc >= 4 ? args[3].toInt() : 3);

    } else if (sub == "monitor") {
      if (argc >= 3 && args[2] == "stop") rfMgr.stopSpectrumMonitor();
      else                                 rfMgr.startSpectrumMonitor();

    } else if (sub == "detect") {
      if (argc >= 3 && args[2] == "stop") rfMgr.stopReplayDetector();
      else                                 rfMgr.startReplayDetector();

    } else if (sub == "jam") {
      if (argc >= 3 && args[2] == "stop") {
        rfMgr.jamStop();
      } else if (argc >= 3) {
        rfMgr.jamStart(args[2].toFloat());
      } else { Serial.println("Usage: rf jam <mhz>  |  rf jam stop"); }

    } else {
      Serial.printf("Unknown RF subcommand: '%s'  — type 'help'\n", sub.c_str());
    }

  // ── System ───────────────────────────────────────────────

  } else if (cmd == "info") {
    Serial.printf("  Firmware  : %s v%s (%s)\n", FW_NAME, FW_VERSION, HW_TARGET);
    Serial.printf("  Chip      : %s rev%d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("  CPU MHz   : %d\n", ESP.getCpuFreqMHz());
    Serial.printf("  Free RAM  : %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Flash     : %d MB\n", ESP.getFlashChipSize() / (1024*1024));
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    Serial.printf("  WiFi MAC  : %02X:%02X:%02X:%02X:%02X:%02X\n",
      mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    esp_read_mac(mac, ESP_MAC_BT);
    Serial.printf("  BT MAC    : %02X:%02X:%02X:%02X:%02X:%02X\n",
      mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    Serial.printf("  RF Freq   : %.3f MHz\n", rfMgr.currentFreq);
    Serial.printf("  RF RSSI   : %d dBm\n",  rfMgr.readRSSI());
    Serial.printf("  Uptime    : %lu ms\n",   millis());

  } else if (cmd == "help" || cmd == "?") {
    printHelp();

  } else {
    Serial.printf("Unknown command: '%s'  — type 'help'\n", cmd.c_str());
  }
}

void menuInit() { Serial.print("> "); }

void menuTick() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      Serial.println();
      handleCommand(_buf);
      _buf = "";
      Serial.print("> ");
    } else if (c == 0x7F || c == '\b') {
      if (_buf.length()) { _buf.remove(_buf.length()-1); Serial.print("\b \b"); }
    } else {
      _buf += c; Serial.print(c);
    }
  }
}
