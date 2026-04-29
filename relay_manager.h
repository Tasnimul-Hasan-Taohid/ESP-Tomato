#pragma once
#include <Arduino.h>
#include "config.h"

// ================================================================
//  RelayManager — controls two relays with toggle, timer, schedule
// ================================================================
struct RelayTimer {
  bool     active;
  bool     turnOn;        // true = turn on after delay, false = turn off
  unsigned long triggerAt;
};

class RelayManager {
public:
  RelayManager();
  void begin();
  void loop();

  // Immediate control
  void setRelay1(bool on);
  void setRelay2(bool on);
  void toggleRelay1();
  void toggleRelay2();

  // Timed control — turns relay on/off after 'delayMs' milliseconds
  void timerRelay1(bool turnOn, unsigned long delayMs);
  void timerRelay2(bool turnOn, unsigned long delayMs);
  void cancelTimer1();
  void cancelTimer2();

  bool getRelay1State() const { return _relay1On; }
  bool getRelay2State() const { return _relay2On; }

  String getStatusJSON() const;

private:
  bool        _relay1On;
  bool        _relay2On;
  RelayTimer  _timer1;
  RelayTimer  _timer2;

  void _applyRelay1(bool on);
  void _applyRelay2(bool on);
};

extern RelayManager Relays;
