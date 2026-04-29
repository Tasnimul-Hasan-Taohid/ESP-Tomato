#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

// ================================================================
//  WiFiManager — handles STA connect, AP fallback, reconnect
// ================================================================
enum class WiFiState {
  IDLE,
  CONNECTING,
  CONNECTED_STA,
  CONNECTED_AP,   // Config AP mode (no STA credentials)
  FAILED
};

class WiFiManager {
public:
  WiFiManager();
  void begin();
  void loop();

  // Connect to saved credentials
  bool connect(const String& ssid, const String& pass);

  // Disconnect and start config AP
  void startConfigAP();
  void stopConfigAP();

  // Status
  WiFiState   getState() const { return _state; }
  bool        isConnected() const { return _state == WiFiState::CONNECTED_STA; }
  bool        isAPMode() const { return _state == WiFiState::CONNECTED_AP; }
  int32_t     getRSSI() const;
  String      getIP() const;
  String      getAPIP() const;
  String      getSSID() const;
  String      getMACAddress() const;
  uint32_t    getUptimeSeconds() const;

  // WiFi scan
  String scanNetworksJSON();

private:
  WiFiState     _state;
  unsigned long _lastRetry;
  unsigned long _connectStart;
  unsigned long _connectedAt;
  bool          _apActive;

  void _setState(WiFiState s);
  bool _tryConnect(const String& ssid, const String& pass);
  void _setupAP();
};

extern WiFiManager WifiMgr;
