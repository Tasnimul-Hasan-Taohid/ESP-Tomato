#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// ================================================================
//  StorageManager — wraps ESP32 Preferences for all settings
// ================================================================
class StorageManager {
public:
  StorageManager();

  void begin();

  // WiFi
  String  getWiFiSSID();
  String  getWiFiPass();
  void    setWiFiSSID(const String& ssid);
  void    setWiFiPass(const String& pass);
  void    clearWiFi();

  // Device
  String  getDeviceName();
  void    setDeviceName(const String& name);
  String  getAdminPass();
  void    setAdminPass(const String& pass);
  String  getTimezone();
  void    setTimezone(const String& tz);

  // MQTT
  bool    getMQTTEnabled();
  void    setMQTTEnabled(bool en);
  String  getMQTTHost();
  void    setMQTTHost(const String& host);
  uint16_t getMQTTPort();
  void    setMQTTPort(uint16_t port);
  String  getMQTTUser();
  void    setMQTTUser(const String& u);
  String  getMQTTPass();
  void    setMQTTPass(const String& p);

  // Relays
  bool    getRelay1State();
  void    setRelay1State(bool state);
  bool    getRelay2State();
  void    setRelay2State(bool state);

  // Boot counter
  uint32_t getBootCount();
  void     incBootCount();

  // Factory reset
  void    factoryReset();

private:
  Preferences _prefs;
  bool        _open;

  void _ensureOpen();
  void _close();
};

extern StorageManager Storage;
