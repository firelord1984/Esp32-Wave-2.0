#pragma once
#include <Arduino.h>
#include "config.h"

// Interrupt-driven rotary encoder with debounce

class Encoder {
public:
  void begin() {
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT,  INPUT_PULLUP);
    pinMode(ENC_SW,  INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENC_CLK), _isrCLK, CHANGE);
    _lastSwState = HIGH;
  }

  // Returns +1, -1, or 0 since last call
  int8_t getDelta() {
    int8_t d = _delta;
    _delta = 0;
    return d;
  }

  // Returns true once per press (debounced)
  bool wasPressed() {
    bool cur = !digitalRead(ENC_SW); // active LOW
    if (cur && !_lastSwState) {
      _lastSwState = cur;
      return true;
    }
    _lastSwState = cur;
    return false;
  }

  static volatile int8_t _delta;

private:
  static void IRAM_ATTR _isrCLK() {
    static uint8_t last = 0;
    uint8_t clk = digitalRead(ENC_CLK);
    uint8_t dt  = digitalRead(ENC_DT);
    if (clk != last) {
      _delta += (clk != dt) ? +1 : -1;
      last = clk;
    }
  }
  bool _lastSwState = false;
};

volatile int8_t Encoder::_delta = 0;
extern Encoder enc;
