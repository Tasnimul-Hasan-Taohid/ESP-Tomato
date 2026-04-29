#pragma once
#include <Arduino.h>
#include "config.h"

// ================================================================
//  LEDManager — non-blocking LED status patterns
// ================================================================
enum class LEDPattern {
  OFF,
  ON,
  BLINK_SLOW,     // 1 Hz — connecting
  BLINK_FAST,     // 5 Hz — error / config mode
  PULSE_3,        // 3 quick blinks — event
  HEARTBEAT,      // 2 quick + pause — connected
};

class LEDManager {
public:
  LEDManager();
  void begin();
  void loop();
  void setPattern(LEDPattern p);
  void blinkN(int n);     // Blink N times then return to previous pattern

private:
  LEDPattern    _pattern;
  LEDPattern    _prevPattern;
  unsigned long _lastToggle;
  bool          _state;
  int           _blinkCount;
  int           _blinkTarget;
  int           _phase;       // For heartbeat / pulse sequences

  void _set(bool on);
};

extern LEDManager LED;
