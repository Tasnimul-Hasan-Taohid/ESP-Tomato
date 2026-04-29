#include "storage.h"

StorageManager Storage;

StorageManager::StorageManager() : _open(false) {}

void StorageManager::begin() {
  _prefs.begin(PREF_NAMESPACE, false);
  _open = true;
  incBootCount();
  Serial.printf("[STORAGE] Boot count: %u\n", getBootCount());
}

// ── Private helpers ───────────────────────────────────────────
void StorageManager::_ensureOpen() {
  if (!_open) { _prefs.begin(PREF_NAMESPACE, false); _open = true; }
}
void StorageManager::_close() {
  _prefs.end(); _open = false;
}

// ── WiFi ──────────────────────────────────────────────────────
String StorageManager::getWiFiSSID() { _ensureOpen(); return _prefs.getString(PREF_WIFI_SSID, ""); }
String StorageManager::getWiFiPass() { _ensureOpen(); return _prefs.getString(PREF_WIFI_PASS, ""); }
void StorageManager::setWiFiSSID(const String& s) { _ensureOpen(); _prefs.putString(PREF_WIFI_SSID, s); }
void StorageManager::setWiFiPass(const String& p) { _ensureOpen(); _prefs.putString(PREF_WIFI_PASS, p); }
void StorageManager::clearWiFi() {
  _ensureOpen();
  _prefs.remove(PREF_WIFI_SSID);
  _prefs.remove(PREF_WIFI_PASS);
  Serial.println("[STORAGE] WiFi credentials cleared");
}

// ── Device ────────────────────────────────────────────────────
String StorageManager::getDeviceName() { _ensureOpen(); return _prefs.getString(PREF_DEVICE_NAME, TOMATO_NAME); }
void StorageManager::setDeviceName(const String& n) { _ensureOpen(); _prefs.putString(PREF_DEVICE_NAME, n); }
String StorageManager::getAdminPass() { _ensureOpen(); return _prefs.getString(PREF_ADMIN_PASS, WEB_AUTH_PASS); }
void StorageManager::setAdminPass(const String& p) { _ensureOpen(); _prefs.putString(PREF_ADMIN_PASS, p); }
String StorageManager::getTimezone() { _ensureOpen(); return _prefs.getString(PREF_TIMEZONE, NTP_TIMEZONE); }
void StorageManager::setTimezone(const String& tz) { _ensureOpen(); _prefs.putString(PREF_TIMEZONE, tz); }

// ── MQTT ──────────────────────────────────────────────────────
bool StorageManager::getMQTTEnabled()              { _ensureOpen(); return _prefs.getBool(PREF_MQTT_EN, false); }
void StorageManager::setMQTTEnabled(bool en)       { _ensureOpen(); _prefs.putBool(PREF_MQTT_EN, en); }
String StorageManager::getMQTTHost()               { _ensureOpen(); return _prefs.getString(PREF_MQTT_HOST, ""); }
void StorageManager::setMQTTHost(const String& h)  { _ensureOpen(); _prefs.putString(PREF_MQTT_HOST, h); }
uint16_t StorageManager::getMQTTPort()             { _ensureOpen(); return _prefs.getUShort(PREF_MQTT_PORT, MQTT_PORT); }
void StorageManager::setMQTTPort(uint16_t p)       { _ensureOpen(); _prefs.putUShort(PREF_MQTT_PORT, p); }
String StorageManager::getMQTTUser()               { _ensureOpen(); return _prefs.getString(PREF_MQTT_USER, ""); }
void StorageManager::setMQTTUser(const String& u)  { _ensureOpen(); _prefs.putString(PREF_MQTT_USER, u); }
String StorageManager::getMQTTPass()               { _ensureOpen(); return _prefs.getString(PREF_MQTT_PASS, ""); }
void StorageManager::setMQTTPass(const String& p)  { _ensureOpen(); _prefs.putString(PREF_MQTT_PASS, p); }

// ── Relays ────────────────────────────────────────────────────
bool StorageManager::getRelay1State()        { _ensureOpen(); return _prefs.getBool(PREF_RELAY1_STATE, false); }
void StorageManager::setRelay1State(bool s)  { _ensureOpen(); _prefs.putBool(PREF_RELAY1_STATE, s); }
bool StorageManager::getRelay2State()        { _ensureOpen(); return _prefs.getBool(PREF_RELAY2_STATE, false); }
void StorageManager::setRelay2State(bool s)  { _ensureOpen(); _prefs.putBool(PREF_RELAY2_STATE, s); }

// ── Boot counter ──────────────────────────────────────────────
uint32_t StorageManager::getBootCount() { _ensureOpen(); return _prefs.getUInt(PREF_BOOT_COUNT, 0); }
void StorageManager::incBootCount()     { _ensureOpen(); _prefs.putUInt(PREF_BOOT_COUNT, getBootCount() + 1); }

// ── Factory reset ─────────────────────────────────────────────
void StorageManager::factoryReset() {
  _ensureOpen();
  _prefs.clear();
  Serial.println("[STORAGE] Factory reset — all settings cleared");
}
