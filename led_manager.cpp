#include "led_manager.h"

LEDManager LED;

LEDManager::LEDManager()
  : _pattern(LEDPattern::OFF), _prevPattern(LEDPattern::OFF),
    _lastToggle(0), _state(false),
    _blinkCount(0), _blinkTarget(0), _phase(0) {}

void LEDManager::begin() {
  pinMode(PIN_LED_STATUS, OUTPUT);
  _set(false);
  Serial.printf("[LED] Init on pin %d\n", PIN_LED_STATUS);
}

void LEDManager::setPattern(LEDPattern p) {
  if (_pattern == p) return;
  _pattern    = p;
  _phase      = 0;
  _blinkCount = 0;
}

void LEDManager::blinkN(int n) {
  _prevPattern  = _pattern;
  _blinkTarget  = n;
  _blinkCount   = 0;
  _pattern      = LEDPattern::PULSE_3; // Reuse pulse logic
  _phase        = 0;
}

void LEDManager::loop() {
  unsigned long now = millis();

  switch (_pattern) {
    case LEDPattern::OFF:
      _set(false);
      break;

    case LEDPattern::ON:
      _set(true);
      break;

    case LEDPattern::BLINK_SLOW:
      if (now - _lastToggle >= 500) {
        _lastToggle = now;
        _set(!_state);
      }
      break;

    case LEDPattern::BLINK_FAST:
      if (now - _lastToggle >= 100) {
        _lastToggle = now;
        _set(!_state);
      }
      break;

    case LEDPattern::HEARTBEAT: {
      // Beat-beat ... pause
      static const int beats[] = {80, 80, 80, 80, 600};
      static const int n = sizeof(beats) / sizeof(beats[0]);
      if (now - _lastToggle >= (unsigned long)beats[_phase]) {
        _lastToggle = now;
        _set(!_state);
        _phase = (_phase + 1) % n;
      }
      break;
    }

    case LEDPattern::PULSE_3: {
      // N quick blinks then stop (or return to prev)
      int target = (_blinkTarget > 0) ? _blinkTarget : 3;
      if (_blinkCount < target * 2) {
        if (now - _lastToggle >= 150) {
          _lastToggle = now;
          _set(!_state);
          _blinkCount++;
        }
      } else {
        // Done
        _set(false);
        if (_blinkTarget > 0) {
          _blinkTarget = 0;
          _pattern = _prevPattern;
        } else {
          delay(400);
          _blinkCount = 0;
        }
      }
      break;
    }
  }
}

void LEDManager::_set(bool on) {
  _state = on;
  // Most ESP32 boards have active-LOW LED
  digitalWrite(PIN_LED_STATUS, on ? LOW : HIGH);
}
