#include "cc1101_manager.h"

// ── SPI Helpers ───────────────────────────────────────────────

void CC1101Manager::_waitMiso() {
  uint32_t t = millis();
  while (digitalRead(CC1101_MISO) && millis() - t < 100);
}

uint8_t CC1101Manager::_strobe(uint8_t cmd) {
  _csLow();
  _waitMiso();
  uint8_t status = _spi->transfer(cmd);
  _csHigh();
  return status;
}

uint8_t CC1101Manager::_readReg(uint8_t addr) {
  _csLow();
  _waitMiso();
  _spi->transfer(addr | CC_READ);
  uint8_t val = _spi->transfer(0x00);
  _csHigh();
  return val;
}

void CC1101Manager::_writeReg(uint8_t addr, uint8_t val) {
  _csLow();
  _waitMiso();
  _spi->transfer(addr | CC_WRITE);
  _spi->transfer(val);
  _csHigh();
}

void CC1101Manager::_readBurst(uint8_t addr, uint8_t* buf, uint8_t len) {
  _csLow();
  _waitMiso();
  _spi->transfer(addr | CC_READ | CC_BURST);
  for (uint8_t i = 0; i < len; i++) buf[i] = _spi->transfer(0x00);
  _csHigh();
}

void CC1101Manager::_writeBurst(uint8_t addr, const uint8_t* buf, uint8_t len) {
  _csLow();
  _waitMiso();
  _spi->transfer(addr | CC_WRITE | CC_BURST);
  for (uint8_t i = 0; i < len; i++) _spi->transfer(buf[i]);
  _csHigh();
}

// ── Frequency Calculation ─────────────────────────────────────
// fcarrier = (FREQ / 2^16) * Fxosc,  Fxosc = 26 MHz

void CC1101Manager::_calcFreqRegs(float mhz, uint8_t& f2, uint8_t& f1, uint8_t& f0) {
  uint32_t reg = (uint32_t)((mhz * 1e6 / 26e6) * (1UL << 16));
  f2 = (reg >> 16) & 0xFF;
  f1 = (reg >>  8) & 0xFF;
  f0 =  reg        & 0xFF;
}

// ── Default Register Settings ─────────────────────────────────
// OOK, 4.8 kBaud, 200 kHz BW, 433.92 MHz

void CC1101Manager::_applyDefaults() {
  _writeReg(CC_IOCFG0,   0x0D); // GDO0: asserts when sync word received
  _writeReg(CC_IOCFG2,   0x0D); // GDO2: asserts when sync word received
  _writeReg(CC_FIFOTHR,  0x47); // RX FIFO threshold = 32 bytes
  _writeReg(CC_PKTCTRL0, 0x00); // Fixed packet length, no CRC, no whitening → raw
  _writeReg(CC_PKTCTRL1, 0x00); // No addr check, no status append
  _writeReg(CC_PKTLEN,   0xFF); // Max packet length
  _writeReg(CC_SYNC1,    0xD3); // Sync word high
  _writeReg(CC_SYNC0,    0x91); // Sync word low
  _writeReg(CC_MCSM0,    0x18); // Auto-calibrate on IDLE→RX/TX
  _writeReg(CC_MCSM1,    0x00); // Return to IDLE after RX/TX
  _writeReg(CC_AGCCTRL2, 0xC7);
  _writeReg(CC_AGCCTRL1, 0x00);
  _writeReg(CC_AGCCTRL0, 0xB2);
  _writeReg(CC_FREND0,   0x11); // OOK front end
  _writeReg(CC_FSCAL3,   0xE9);
  _writeReg(CC_FSCAL2,   0x2A);
  _writeReg(CC_FSCAL1,   0x00);
  _writeReg(CC_FSCAL0,   0x1F);
  _writeReg(CC_TEST2,    0x81);
  _writeReg(CC_TEST1,    0x35);
  _writeReg(CC_TEST0,    0x09);
  _writeReg(CC_FSCTRL1,  0x06); // IF = 152 kHz
  _writeReg(CC_FSCTRL0,  0x00);

  setFrequency(CC1101_DEFAULT_FREQ);
  setModulation(MOD_ASK);
  setBandwidth(200);
  setDataRate(CC1101_DEFAULT_DRATE);
  setDeviation(CC1101_DEFAULT_DEVN);
  setOutputPower(10);
}

// ── Lifecycle ────────────────────────────────────────────────

bool CC1101Manager::begin() {
  pinMode(CC1101_CS,   OUTPUT);
  pinMode(CC1101_GDO0, INPUT);
  pinMode(CC1101_GDO2, INPUT);
  digitalWrite(CC1101_CS, HIGH);

  _spi = new SPIClass(HSPI);
  _spi->begin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);
  _spi->setFrequency(4000000); // 4 MHz — CC1101 max is ~10 MHz

  reset();
  delay(10);

  uint8_t partnum = _readReg(CC_PARTNUM | CC_BURST);
  uint8_t version = _readReg(CC_VERSION  | CC_BURST);

  Serial.printf("[CC1101] Part: 0x%02X  Version: 0x%02X\n", partnum, version);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("[CC1101] ERROR: No response — check SPI wiring!");
    return false;
  }

  _applyDefaults();
  _initialized = true;
  clearAllSignals();
  Serial.printf("[CC1101] Ready at %.2f MHz\n", currentFreq);
  return true;
}

void CC1101Manager::reset() {
  _csLow();  delayMicroseconds(10);
  _csHigh(); delayMicroseconds(40);
  _csLow();
  _waitMiso();
  _spi->transfer(CC_SRES);
  _waitMiso();
  _csHigh();
  delay(5);
}

void CC1101Manager::idle() {
  _strobe(CC_SIDLE);
  delayMicroseconds(100);
}

void CC1101Manager::flushFIFOs() {
  idle();
  _strobe(CC_SFRX);
  _strobe(CC_SFTX);
  delayMicroseconds(100);
}

// ── Configuration ────────────────────────────────────────────

void CC1101Manager::setFrequency(float mhz) {
  uint8_t f2, f1, f0;
  _calcFreqRegs(mhz, f2, f1, f0);
  idle();
  _writeReg(CC_FREQ2, f2);
  _writeReg(CC_FREQ1, f1);
  _writeReg(CC_FREQ0, f0);
  _strobe(CC_SCAL);
  delay(2);
  currentFreq = mhz;
}

float CC1101Manager::getFrequency() { return currentFreq; }

void CC1101Manager::setModulation(CC1101Modulation mod) {
  uint8_t mdm = _readReg(CC_MDMCFG2) & 0x8F;
  _writeReg(CC_MDMCFG2, mdm | (uint8_t)mod);
  // OOK requires FREND0[0] = 1 for ASK shaping
  if (mod == MOD_ASK) _writeReg(CC_FREND0, 0x11);
  else                 _writeReg(CC_FREND0, 0x10);
}

void CC1101Manager::setBandwidth(uint16_t bw_khz) {
  // BW = Fxosc / (8 × (4 + CHANBW_M) × 2^CHANBW_E)
  struct { uint16_t bw; uint8_t chanbw; } table[] = {
    {812, 0x00}, {650, 0x10}, {541, 0x20}, {464, 0x30},
    {405, 0x40}, {325, 0x50}, {270, 0x60}, {232, 0x70},
    {203, 0x80}, {162, 0x90}, {135, 0xA0}, {116, 0xB0},
    {102, 0xC0}, { 81, 0xD0}, { 68, 0xE0}, { 58, 0xF0}
  };
  uint8_t best = 0xC0;
  uint16_t bestDiff = 0xFFFF;
  for (auto& e : table) {
    uint16_t d = abs((int)e.bw - (int)bw_khz);
    if (d < bestDiff) { bestDiff = d; best = e.chanbw; }
  }
  uint8_t mdm4 = _readReg(CC_MDMCFG4) & 0x0F;
  _writeReg(CC_MDMCFG4, mdm4 | best);
}

void CC1101Manager::setDataRate(float kbaud) {
  // DR = (256 + DRATE_M) × 2^DRATE_E × Fxosc / 2^28
  float dr = kbaud * 1000.0f;
  uint8_t e = 0;
  while (dr > 26e6 / 256.0f && e < 15) { dr /= 2.0f; e++; }
  uint8_t m = (uint8_t)round((dr / 26e6 * 256.0f) - 256);
  uint8_t mdm4 = _readReg(CC_MDMCFG4) & 0xF0;
  _writeReg(CC_MDMCFG4, mdm4 | (e & 0x0F));
  _writeReg(CC_MDMCFG3, m);
}

void CC1101Manager::setDeviation(float khz) {
  // DEV = (8 + DEVIATION_M) × 2^DEVIATION_E × Fxosc / 2^17
  float dev = khz * 1000.0f;
  uint8_t e = 0;
  while (dev > 26e6 * 9.0f / (1UL << 17) && e < 7) { dev /= 2.0f; e++; }
  uint8_t m = (uint8_t)round(dev * (1UL << 17) / 26e6 / 8.0f) - 1;
  _writeReg(CC_DEVIATN, ((e & 0x07) << 4) | (m & 0x07));
}

void CC1101Manager::setOutputPower(int8_t dbm) {
  // PA table values for 433 MHz (OOK uses PATABLE[0]=0x00, PATABLE[1]=power)
  uint8_t pa;
  if      (dbm >= 10) pa = 0xC0;
  else if (dbm >=  7) pa = 0xC8;
  else if (dbm >=  5) pa = 0x84;
  else if (dbm >=  0) pa = 0x60;
  else if (dbm >= -6) pa = 0x34;
  else if (dbm >=-10) pa = 0x26;
  else if (dbm >=-15) pa = 0x1D;
  else if (dbm >=-20) pa = 0x17;
  else                pa = 0x03;

  uint8_t patable[8] = {0x00, pa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  _writeBurst(0x3E, patable, 8); // PA table write
}

void CC1101Manager::setSyncWord(uint8_t sync1, uint8_t sync0) {
  _writeReg(CC_SYNC1, sync1);
  _writeReg(CC_SYNC0, sync0);
}

void CC1101Manager::setPacketMode(bool variable) {
  _writeReg(CC_PKTCTRL0, variable ? 0x01 : 0x00);
}

void CC1101Manager::printConfig() {
  Serial.println("[CC1101] Current Config:");
  Serial.printf("  Frequency : %.3f MHz\n", currentFreq);
  uint8_t mod = (_readReg(CC_MDMCFG2) >> 4) & 0x07;
  const char* modNames[] = {"2-FSK","GFSK","(n/a)","ASK/OOK","4-FSK","(n/a)","(n/a)","MSK"};
  Serial.printf("  Modulation: %s\n", modNames[mod & 7]);
  Serial.printf("  RSSI      : %d dBm\n", readRSSI());
  Serial.printf("  MDMCFG4   : 0x%02X\n", _readReg(CC_MDMCFG4));
  Serial.printf("  MDMCFG3   : 0x%02X\n", _readReg(CC_MDMCFG3));
}

// ── RX / TX ───────────────────────────────────────────────────

int CC1101Manager::receiveRaw(uint8_t* buf, uint16_t maxLen, uint32_t timeoutMs) {
  flushFIFOs();
  _strobe(CC_SRX);

  uint32_t t = millis();
  // Wait for GDO0 to assert (sync word / carrier detect)
  while (!digitalRead(CC1101_GDO0)) {
    if (millis() - t > timeoutMs) {
      idle();
      Serial.println("[CC1101] RX timeout.");
      return -1;
    }
  }

  uint16_t total = 0;
  // Drain FIFO while GDO0 asserted
  while (digitalRead(CC1101_GDO0) && total < maxLen) {
    uint8_t avail = _readReg(CC_RXBYTES | CC_BURST) & 0x7F;
    if (avail > 0) {
      uint8_t chunk = min((uint8_t)avail, (uint8_t)(maxLen - total));
      _readBurst(CC_RXFIFO | CC_BURST, buf + total, chunk);
      total += chunk;
    }
  }
  // Drain any remaining
  uint8_t avail = _readReg(CC_RXBYTES | CC_BURST) & 0x7F;
  if (avail && total < maxLen) {
    _readBurst(CC_RXFIFO | CC_BURST, buf + total, avail);
    total += avail;
  }

  idle();
  return total;
}

void CC1101Manager::transmitRaw(const uint8_t* buf, uint16_t len) {
  idle();
  flushFIFOs();

  uint16_t offset = 0;
  while (offset < len) {
    uint8_t chunk = min((uint16_t)60, (uint16_t)(len - offset));
    _writeBurst(CC_TXFIFO | CC_BURST, buf + offset, chunk);
    offset += chunk;

    if (offset == chunk) {   // first chunk — kick TX
      _strobe(CC_STX);
    }
    // Wait for FIFO to drain before filling again
    while ((_readReg(CC_TXBYTES | CC_BURST) & 0x7F) > 16) delayMicroseconds(100);
  }

  // Wait for TX done (GDO0 de-assert)
  uint32_t t = millis();
  while (digitalRead(CC1101_GDO0) && millis() - t < 1000);
  idle();
}

// ── Record & Replay ──────────────────────────────────────────

bool CC1101Manager::recordSignal(uint8_t slot, const char* label) {
  if (slot >= CC1101_MAX_RECORDINGS) { Serial.println("[CC1101] Invalid slot."); return false; }

  Serial.printf("[CC1101] Recording to slot %d at %.3f MHz — waiting for signal...\n", slot, currentFreq);
  RFSignal& s = signals[slot];

  int len = receiveRaw(s.data, CC1101_MAX_SIGNAL_LEN, CC1101_RX_TIMEOUT_MS);
  if (len <= 0) { s.valid = false; return false; }

  s.len        = len;
  s.frequency  = currentFreq;
  s.modulation = (_readReg(CC_MDMCFG2) >> 4) & 0x07;
  s.valid      = true;
  snprintf(s.label, sizeof(s.label), "%s", label ? label : "recorded");

  Serial.printf("[CC1101] Slot %d: %d bytes captured (\"%s\")\n", slot, len, s.label);
  return true;
}

void CC1101Manager::replaySignal(uint8_t slot, uint8_t times, uint16_t delayMs) {
  if (slot >= CC1101_MAX_RECORDINGS || !signals[slot].valid) {
    Serial.printf("[CC1101] Slot %d is empty.\n", slot);
    return;
  }
  RFSignal& s = signals[slot];
  setFrequency(s.frequency);

  Serial.printf("[CC1101] Replaying slot %d (\"%s\") × %d @ %.3f MHz\n",
    slot, s.label, times, s.frequency);

  for (uint8_t i = 0; i < times; i++) {
    transmitRaw(s.data, s.len);
    if (delayMs) delay(delayMs);
    if (Serial.available()) break;
  }
  Serial.println("[CC1101] Replay done.");
}

void CC1101Manager::replayAll(uint8_t times) {
  for (uint8_t i = 0; i < CC1101_MAX_RECORDINGS; i++) {
    if (signals[i].valid) replaySignal(i, times, 100);
  }
}

// ── Frequency Scanner ─────────────────────────────────────────

void CC1101Manager::scanFrequencies(float startMhz, float endMhz, float stepMhz) {
  Serial.printf("[CC1101] Scanning %.2f–%.2f MHz step %.2f MHz\n",
    startMhz, endMhz, stepMhz);
  Serial.println("  Freq(MHz)   RSSI(dBm)  Level");

  int8_t peakRSSI = -120;
  float  peakFreq = startMhz;

  for (float f = startMhz; f <= endMhz; f += stepMhz) {
    setFrequency(f);
    _strobe(CC_SRX);
    delay(5);
    int8_t rssi = readRSSI();
    idle();

    char bar[21] = "                    ";
    int  barLen  = map(constrain(rssi, -120, -30), -120, -30, 0, 20);
    for (int b = 0; b < barLen; b++) bar[b] = '#';

    Serial.printf("  %9.3f   %4d       %s\n", f, rssi, bar);

    if (rssi > peakRSSI) { peakRSSI = rssi; peakFreq = f; }
    if (Serial.available()) break;
  }
  Serial.printf("[CC1101] Peak: %.3f MHz at %d dBm\n", peakFreq, peakRSSI);
  setFrequency(currentFreq); // restore
}

// ── Jam ───────────────────────────────────────────────────────
// NOTE: RF jamming is illegal in most jurisdictions.
// Only use on your own equipment in a shielded environment.

void CC1101Manager::jamStart(float mhz) {
  Serial.println("[CC1101] WARNING: Jamming is illegal outside lab/shielded environments!");
  setFrequency(mhz);
  setModulation(MOD_ASK);
  setOutputPower(10);

  // Fill TX FIFO with 0xFF (continuous carrier approximation with OOK)
  uint8_t noise[64];
  memset(noise, 0xFF, sizeof(noise));
  flushFIFOs();
  _writeReg(CC_PKTCTRL0, 0x02); // infinite packet length mode
  _writeBurst(CC_TXFIFO | CC_BURST, noise, sizeof(noise));
  _strobe(CC_STX);
  jamming = true;
  Serial.printf("[CC1101] Jamming %.3f MHz — 'jam stop' to end.\n", mhz);
}

void CC1101Manager::jamStop() {
  idle();
  flushFIFOs();
  _writeReg(CC_PKTCTRL0, 0x00); // restore fixed packet mode
  jamming = false;
  Serial.println("[CC1101] Jamming stopped.");
}

// ── Brute-Force (Fixed Codes) ─────────────────────────────────
// De Bruijn sequence covers all N-bit patterns in one transmission.
// Relevant for fixed-code remotes (not rolling-code/HiTag2).

void CC1101Manager::_deBruijn(uint8_t* seq, uint8_t& outLen, uint8_t bits) {
  // Simple de Bruijn B(2,n) generation via LFSR
  outLen = (1 << bits) + bits - 1; // length = 2^n + (n-1) for wraparound
  uint16_t lfsr = 1;
  for (uint8_t i = 0; i < outLen; i++) {
    seq[i / 8] |= ((lfsr & 1) << (7 - (i % 8)));
    uint16_t feedback = ((lfsr >> (bits-1)) ^ lfsr) & 1;
    lfsr = (lfsr >> 1) | (feedback << (bits - 1));
  }
}

void CC1101Manager::bruteForce(uint8_t bits, uint8_t times) {
  if (bits < 4 || bits > 12) {
    Serial.println("[CC1101] bits must be 4–12 for brute force.");
    return;
  }
  uint8_t seq[512] = {0};
  uint8_t seqLen   = 0;
  _deBruijn(seq, seqLen, bits);

  Serial.printf("[CC1101] Brute-force: %d-bit de Bruijn seq (%d bytes) × %d\n",
    bits, seqLen, times);

  for (uint8_t t = 0; t < times; t++) {
    transmitRaw(seq, seqLen);
    delay(50);
  }
  Serial.println("[CC1101] Brute-force done.");
}

// ── Spectrum Monitor ─────────────────────────────────────────

void CC1101Manager::startSpectrumMonitor() {
  Serial.printf("[CC1101] Spectrum monitor on %.3f MHz — 'rf monitor stop' to end.\n", currentFreq);
  _strobe(CC_SRX);
  monitoring = true;
}

void CC1101Manager::stopSpectrumMonitor() {
  idle();
  monitoring = false;
  Serial.println("[CC1101] Monitor stopped.");
}

void CC1101Manager::spectrumTick() {
  if (!monitoring) return;
  static uint32_t last = 0;
  if (millis() - last < 200) return;
  last = millis();
  int8_t rssi = readRSSI();
  Serial.printf("  [RF] %.3f MHz  RSSI: %d dBm\n", currentFreq, rssi);
}

// ── Replay Detector ───────────────────────────────────────────

void CC1101Manager::startReplayDetector() {
  _replayDetecting = true;
  flushFIFOs();
  _strobe(CC_SRX);
  Serial.printf("[BLUE TEAM] RF replay detector active on %.3f MHz\n", currentFreq);
}

void CC1101Manager::stopReplayDetector() {
  _replayDetecting = false;
  idle();
  Serial.println("[BLUE TEAM] RF detector stopped.");
}

void CC1101Manager::replayDetectorTick() {
  if (!_replayDetecting) return;
  static uint8_t lastSig[64] = {0};
  static bool    hasLast = false;

  uint8_t rxbytes = _readReg(CC_RXBYTES | CC_BURST) & 0x7F;
  if (rxbytes == 0) return;

  uint8_t buf[64];
  uint8_t len = min((uint8_t)64, rxbytes);
  _readBurst(CC_RXFIFO | CC_BURST, buf, len);

  if (hasLast && memcmp(buf, lastSig, len) == 0) {
    Serial.printf("  [!!!] REPLAY DETECTED on %.3f MHz — identical %d-byte packet!\n",
      currentFreq, len);
  }
  memcpy(lastSig, buf, len);
  hasLast = true;

  flushFIFOs();
  _strobe(CC_SRX);
}

// ── Utilities ─────────────────────────────────────────────────

int8_t CC1101Manager::readRSSI() {
  uint8_t raw = _readReg(CC_RSSI | CC_BURST);
  int8_t rssi;
  if (raw >= 128) rssi = ((int)raw - 256) / 2 - 74;
  else            rssi = raw / 2 - 74;
  return rssi;
}

uint8_t CC1101Manager::readLQI() {
  return _readReg(CC_LQI | CC_BURST) & 0x7F;
}

void CC1101Manager::printSignals() {
  bool any = false;
  Serial.println("  Slot  Freq(MHz)  Bytes  Modulation  Label");
  Serial.println("  ----  ---------  -----  ----------  -----");
  for (int i = 0; i < CC1101_MAX_RECORDINGS; i++) {
    if (signals[i].valid) {
      const char* modNames[] = {"2-FSK","GFSK","(n/a)","ASK/OOK","4-FSK","(n/a)","(n/a)","MSK"};
      Serial.printf("  %-4d  %-9.3f  %-5d  %-10s  %s\n",
        i, signals[i].frequency, signals[i].len,
        modNames[signals[i].modulation & 7], signals[i].label);
      any = true;
    }
  }
  if (!any) Serial.println("  (no recordings)");
}

void CC1101Manager::clearSignal(uint8_t slot) {
  if (slot < CC1101_MAX_RECORDINGS) {
    signals[slot].valid = false;
    Serial.printf("[CC1101] Slot %d cleared.\n", slot);
  }
}

void CC1101Manager::clearAllSignals() {
  for (int i = 0; i < CC1101_MAX_RECORDINGS; i++) signals[i].valid = false;
}

CC1101Manager rfMgr;
