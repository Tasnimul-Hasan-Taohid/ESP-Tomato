#include "wifi_manager.h"
#include "storage.h"

WiFiManager WifiMgr;

WiFiManager::WiFiManager()
  : _state(WiFiState::IDLE), _lastRetry(0),
    _connectStart(0), _connectedAt(0), _apActive(false) {}

void WiFiManager::begin() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.setHostname(Storage.getDeviceName().c_str());

  String ssid = Storage.getWiFiSSID();
  String pass = Storage.getWiFiPass();

  if (ssid.length() > 0) {
    Serial.printf("[WiFi] Connecting to: %s\n", ssid.c_str());
    _tryConnect(ssid, pass);
  } else {
    Serial.println("[WiFi] No credentials — starting config AP");
    startConfigAP();
  }
}

void WiFiManager::loop() {
  // Auto-reconnect if STA drops
  if (_state == WiFiState::CONNECTED_STA && WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connection lost — scheduling retry");
    _setState(WiFiState::FAILED);
    _lastRetry = millis();
  }

  if (_state == WiFiState::FAILED) {
    if (millis() - _lastRetry >= WIFI_RETRY_INTERVAL) {
      Serial.println("[WiFi] Retrying...");
      String ssid = Storage.getWiFiSSID();
      String pass = Storage.getWiFiPass();
      if (ssid.length() > 0) {
        _tryConnect(ssid, pass);
      } else {
        startConfigAP();
      }
      _lastRetry = millis();
    }
  }

  // Timeout connecting
  if (_state == WiFiState::CONNECTING) {
    if (millis() - _connectStart > WIFI_CONNECT_TIMEOUT) {
      Serial.println("[WiFi] Connection timeout");
      WiFi.disconnect();
      _setState(WiFiState::FAILED);
      _lastRetry = millis();
    } else if (WiFi.status() == WL_CONNECTED) {
      _connectedAt = millis();
      _setState(WiFiState::CONNECTED_STA);
      Serial.printf("[WiFi] Connected! IP: %s  RSSI: %d dBm\n",
        WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
  }
}

bool WiFiManager::connect(const String& ssid, const String& pass) {
  Storage.setWiFiSSID(ssid);
  Storage.setWiFiPass(pass);
  return _tryConnect(ssid, pass);
}

bool WiFiManager::_tryConnect(const String& ssid, const String& pass) {
  WiFi.disconnect(false);
  delay(100);
  WiFi.mode(WIFI_AP_STA);

  if (pass.length() > 0) {
    WiFi.begin(ssid.c_str(), pass.c_str());
  } else {
    WiFi.begin(ssid.c_str());
  }

  _connectStart = millis();
  _setState(WiFiState::CONNECTING);
  return true;
}

void WiFiManager::startConfigAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_GATEWAY, WIFI_AP_SUBNET);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  _apActive = true;
  _setState(WiFiState::CONNECTED_AP);
  Serial.printf("[WiFi] Config AP started: %s  IP: %s\n",
    WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
}

void WiFiManager::stopConfigAP() {
  WiFi.softAPdisconnect(true);
  _apActive = false;
}

int32_t WiFiManager::getRSSI() const {
  return WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
}

String WiFiManager::getIP() const {
  return WiFi.localIP().toString();
}

String WiFiManager::getAPIP() const {
  return WiFi.softAPIP().toString();
}

String WiFiManager::getSSID() const {
  return WiFi.SSID();
}

String WiFiManager::getMACAddress() const {
  return WiFi.macAddress();
}

uint32_t WiFiManager::getUptimeSeconds() const {
  if (_connectedAt == 0) return 0;
  return (millis() - _connectedAt) / 1000;
}

String WiFiManager::scanNetworksJSON() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\","
          + "\"rssi\":"   + WiFi.RSSI(i) + ","
          + "\"enc\":"    + (WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();
  return json;
}

void WiFiManager::_setState(WiFiState s) {
  _state = s;
}
