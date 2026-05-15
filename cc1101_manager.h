#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "config.h"

// ── CC1101 Register Map ──────────────────────────────────────
#define CC_IOCFG2    0x00
#define CC_IOCFG1    0x01
#define CC_IOCFG0    0x02
#define CC_FIFOTHR   0x03
#define CC_SYNC1     0x04
#define CC_SYNC0     0x05
#define CC_PKTLEN    0x06
#define CC_PKTCTRL1  0x07
#define CC_PKTCTRL0  0x08
#define CC_ADDR      0x09
#define CC_CHANNR    0x0A
#define CC_FSCTRL1   0x0B
#define CC_FSCTRL0   0x0C
#define CC_FREQ2     0x0D
#define CC_FREQ1     0x0E
#define CC_FREQ0     0x0F
#define CC_MDMCFG4   0x10
#define CC_MDMCFG3   0x11
#define CC_MDMCFG2   0x12
#define CC_MDMCFG1   0x13
#define CC_MDMCFG0   0x14
#define CC_DEVIATN   0x15
#define CC_MCSM2     0x16
#define CC_MCSM1     0x17
#define CC_MCSM0     0x18
#define CC_FOCCFG    0x19
#define CC_BSCFG     0x1A
#define CC_AGCCTRL2  0x1B
#define CC_AGCCTRL1  0x1C
#define CC_AGCCTRL0  0x1D
#define CC_WOREVT1   0x1E
#define CC_WOREVT0   0x1F
#define CC_WORCTRL   0x20
#define CC_FREND1    0x21
#define CC_FREND0    0x22
#define CC_FSCAL3    0x23
#define CC_FSCAL2    0x24
#define CC_FSCAL1    0x25
#define CC_FSCAL0    0x26
#define CC_RCCTRL1   0x27
#define CC_RCCTRL0   0x28
#define CC_FSTEST    0x29
#define CC_TEST2     0x2C
#define CC_TEST1     0x2D
#define CC_TEST0     0x2E

// Status registers (read with burst bit)
#define CC_PARTNUM   0x30
#define CC_VERSION   0x31
#define CC_FREQEST   0x32
#define CC_LQI       0x33
#define CC_RSSI      0x34
#define CC_MARCSTATE 0x35
#define CC_PKTSTATUS 0x38
#define CC_RXBYTES   0x3B
#define CC_TXBYTES   0x3A

// TX FIFO / RX FIFO
#define CC_TXFIFO    0x3F
#define CC_RXFIFO    0x3F

// Command strobes
#define CC_SRES      0x30
#define CC_SFSTXON   0x31
#define CC_SXOFF     0x32
#define CC_SCAL      0x33
#define CC_SRX       0x34
#define CC_STX       0x35
#define CC_SIDLE     0x36
#define CC_SWOR      0x38
#define CC_SPWD      0x39
#define CC_SFRX      0x3A
#define CC_SFTX      0x3B
#define CC_SWORRST   0x3C
#define CC_SNOP      0x3D

// SPI access bits
#define CC_READ      0x80
#define CC_BURST     0x40
#define CC_WRITE     0x00

// Modulation modes
enum CC1101Modulation {
  MOD_2FSK   = 0x00,
  MOD_GFSK   = 0x10,
  MOD_ASK    = 0x30,  // OOK
  MOD_4FSK   = 0x40,
  MOD_MSK    = 0x70
};

// ── Signal Recording ─────────────────────────────────────────
struct RFSignal {
  uint8_t  data[CC1101_MAX_SIGNAL_LEN];
  uint16_t len;
  float    frequency;
  uint8_t  modulation;
  char     label[24];
  bool     valid;
};

// ── CC1101 Manager ───────────────────────────────────────────
class CC1101Manager {
public:
  bool  begin();
  void  reset();

  // ── Configuration ────────────────────────────────────────
  void  setFrequency(float mhz);
  float getFrequency();
  void  setModulation(CC1101Modulation mod);
  void  setBandwidth(uint16_t bw_khz);    // 58/100/200/325/400/540/650/800
  void  setDataRate(float kbaud);
  void  setDeviation(float khz);
  void  setOutputPower(int8_t dbm);       // -30 to +10 dBm
  void  setSyncWord(uint8_t sync1, uint8_t sync0);
  void  setPacketMode(bool variable);     // true=variable length, false=raw
  void  printConfig();

  // ── Sub-GHz Red Team ─────────────────────────────────────

  // Receive raw OOK/FSK samples into buffer, returns actual length
  int   receiveRaw(uint8_t* buf, uint16_t maxLen, uint32_t timeoutMs = CC1101_RX_TIMEOUT_MS);

  // Transmit raw buffer
  void  transmitRaw(const uint8_t* buf, uint16_t len);

  // Record a signal to internal slot (0-based index)
  bool  recordSignal(uint8_t slot, const char* label = nullptr);

  // Replay stored signal
  void  replaySignal(uint8_t slot, uint8_t times = 1, uint16_t delayMs = 50);

  // Replay all slots in sequence
  void  replayAll(uint8_t times = 1);

  // Frequency scanner — sweep and print RSSI per channel
  void  scanFrequencies(float startMhz, float endMhz, float stepMhz = 0.5f);

  // Jam a frequency by continuous TX of noise (use only on your own equipment)
  void  jamStart(float mhz);
  void  jamStop();

  // Brute-force common rolling code lengths (de Bruijn sequence)
  // For research on fixed-code systems (garage doors, barriers)
  void  bruteForce(uint8_t bits, uint8_t times = 3);

  // ── Sub-GHz Blue Team ────────────────────────────────────

  // Passive RF spectrum monitor — print RSSI on current freq
  void  startSpectrumMonitor();
  void  stopSpectrumMonitor();
  void  spectrumTick();

  // Detect replay attacks (listen for repeated identical packets)
  void  startReplayDetector();
  void  stopReplayDetector();
  void  replayDetectorTick();

  // ── Utilities ────────────────────────────────────────────
  int8_t  readRSSI();
  uint8_t readLQI();
  void    idle();
  void    flushFIFOs();
  void    printSignals();
  void    clearSignal(uint8_t slot);
  void    clearAllSignals();

  // ── State ────────────────────────────────────────────────
  RFSignal signals[CC1101_MAX_RECORDINGS];
  float    currentFreq   = CC1101_DEFAULT_FREQ;
  bool     jamming       = false;
  bool     monitoring    = false;

private:
  SPIClass* _spi = nullptr;
  bool      _initialized = false;
  bool      _replayDetecting = false;

  void    _csLow()  { digitalWrite(CC1101_CS, LOW);  }
  void    _csHigh() { digitalWrite(CC1101_CS, HIGH); }
  void    _waitMiso();

  uint8_t _readReg(uint8_t addr);
  void    _writeReg(uint8_t addr, uint8_t val);
  void    _readBurst(uint8_t addr, uint8_t* buf, uint8_t len);
  void    _writeBurst(uint8_t addr, const uint8_t* buf, uint8_t len);
  uint8_t _strobe(uint8_t cmd);

  void    _calcFreqRegs(float mhz, uint8_t& f2, uint8_t& f1, uint8_t& f0);
  void    _applyDefaults();

  // Rolling code brute-force helpers
  void    _deBruijn(uint8_t* seq, uint8_t& len, uint8_t bits);
};

extern CC1101Manager rfMgr;
