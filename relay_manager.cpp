#include "relay_manager.h"
#include "storage.h"

RelayManager Relays;

RelayManager::RelayManager()
  : _relay1On(false), _relay2On(false) {
  memset(&_timer1, 0, sizeof(_timer1));
  memset(&_timer2, 0, sizeof(_timer2));
}

void RelayManager::begin() {
#if FEATURE_RELAY
  pinMode(PIN_RELAY_1, OUTPUT);
  pinMode(PIN_RELAY_2, OUTPUT);

  // Restore last saved state
  _relay1On = Storage.getRelay1State();
  _relay2On = Storage.getRelay2State();
  _applyRelay1(_relay1On);
  _applyRelay2(_relay2On);

  Serial.printf("[RELAY] Init — R1:%s  R2:%s\n",
    _relay1On ? "ON" : "OFF", _relay2On ? "ON" : "OFF");
#else
  Serial.println("[RELAY] Feature disabled — set FEATURE_RELAY=true in config.h");
#endif
}

void RelayManager::loop() {
  unsigned long now = millis();

  if (_timer1.active && now >= _timer1.triggerAt) {
    _timer1.active = false;
    setRelay1(_timer1.turnOn);
    Serial.printf("[RELAY] Timer R1 fired → %s\n", _timer1.turnOn ? "ON" : "OFF");
  }

  if (_timer2.active && now >= _timer2.triggerAt) {
    _timer2.active = false;
    setRelay2(_timer2.turnOn);
    Serial.printf("[RELAY] Timer R2 fired → %s\n", _timer2.turnOn ? "ON" : "OFF");
  }
}

void RelayManager::setRelay1(bool on) {
  _relay1On = on;
  _applyRelay1(on);
  Storage.setRelay1State(on);
}

void RelayManager::setRelay2(bool on) {
  _relay2On = on;
  _applyRelay2(on);
  Storage.setRelay2State(on);
}

void RelayManager::toggleRelay1() { setRelay1(!_relay1On); }
void RelayManager::toggleRelay2() { setRelay2(!_relay2On); }

void RelayManager::timerRelay1(bool turnOn, unsigned long delayMs) {
  _timer1.active    = true;
  _timer1.turnOn    = turnOn;
  _timer1.triggerAt = millis() + delayMs;
  Serial.printf("[RELAY] R1 timer set: %s in %lu ms\n", turnOn ? "ON" : "OFF", delayMs);
}

void RelayManager::timerRelay2(bool turnOn, unsigned long delayMs) {
  _timer2.active    = true;
  _timer2.turnOn    = turnOn;
  _timer2.triggerAt = millis() + delayMs;
  Serial.printf("[RELAY] R2 timer set: %s in %lu ms\n", turnOn ? "ON" : "OFF", delayMs);
}

void RelayManager::cancelTimer1() { _timer1.active = false; }
void RelayManager::cancelTimer2() { _timer2.active = false; }

void RelayManager::_applyRelay1(bool on) {
#if FEATURE_RELAY
  digitalWrite(PIN_RELAY_1, on ? HIGH : LOW);
#endif
}

void RelayManager::_applyRelay2(bool on) {
#if FEATURE_RELAY
  digitalWrite(PIN_RELAY_2, on ? HIGH : LOW);
#endif
}

String RelayManager::getStatusJSON() const {
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{"
    "\"relay1\":%s,"
    "\"relay2\":%s,"
    "\"timer1Active\":%s,"
    "\"timer2Active\":%s,"
    "\"timer1Ms\":%lu,"
    "\"timer2Ms\":%lu"
    "}",
    _relay1On ? "true" : "false",
    _relay2On ? "true" : "false",
    _timer1.active ? "true" : "false",
    _timer2.active ? "true" : "false",
    _timer1.active ? (unsigned long)max(0L, (long)(_timer1.triggerAt - millis())) : 0UL,
    _timer2.active ? (unsigned long)max(0L, (long)(_timer2.triggerAt - millis())) : 0UL
  );
  return String(buf);
}
