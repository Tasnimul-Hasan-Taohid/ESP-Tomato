#include "web_server.h"
#include "wifi_manager.h"
#include "system_monitor.h"
#include "relay_manager.h"
#include "mqtt_manager.h"
#include "storage.h"
#include "led_manager.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

TomatoWebServer WebServer;

// ── HTML helpers ─────────────────────────────────────────────
static const char* HTML_HEAD = R"html(<!DOCTYPE html>
<html lang="en">
<head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Tomato</title>
<link rel="stylesheet" href="/style.css">
<script src="/app.js" defer></script>
</head><body>
<nav>
  <a href="/" class="brand">🍅 ESP32 Tomato</a>
  <div class="nav-links">
    <a href="/">Dashboard</a>
    <a href="/relays">Relays</a>
    <a href="/wifi">WiFi</a>
    <a href="/config">Config</a>
    <a href="/logs">Logs</a>
  </div>
</nav>
<div class="container">
)html";

static const char* HTML_FOOT = R"html(
</div>
</body></html>
)html";

// ─────────────────────────────────────────────────────────────
TomatoWebServer::TomatoWebServer()
  : _server(WEB_PORT), _ws("/ws"), _lastBroadcast(0) {}

void TomatoWebServer::begin() {
  _setupWSHandler();
  _setupStaticRoutes();
  _setupAPIRoutes();
  _setupConfigRoutes();

  _server.addHandler(&_ws);

  // 404 handler
  _server.onNotFound([](AsyncWebServerRequest* req) {
    req->send(404, "text/plain", "Not found: " + req->url());
  });

  _server.begin();
  Serial.printf("[WEB] Server started on port %d\n", WEB_PORT);
  SysMon.addLog("Web server started");
}

void TomatoWebServer::loop() {
  // Broadcast stats via WebSocket every 2 seconds
  if (millis() - _lastBroadcast >= 2000 && _ws.count() > 0) {
    _lastBroadcast = millis();
    _broadcastStats();
  }
  _ws.cleanupClients();
}

// ── Auth ─────────────────────────────────────────────────────
bool TomatoWebServer::_authenticate(AsyncWebServerRequest* req) {
  if (!req->authenticate(WEB_AUTH_USER, Storage.getAdminPass().c_str())) {
    req->requestAuthentication();
    return false;
  }
  return true;
}

void TomatoWebServer::_sendJSON(AsyncWebServerRequest* req, const String& json, int code) {
  req->send(code, "application/json", json);
}

void TomatoWebServer::_sendError(AsyncWebServerRequest* req, const String& msg, int code) {
  _sendJSON(req, "{\"error\":\"" + msg + "\"}", code);
}

void TomatoWebServer::_redirectTo(AsyncWebServerRequest* req, const String& url) {
  req->redirect(url);
}

// ── Static routes ─────────────────────────────────────────────
void TomatoWebServer::_setupStaticRoutes() {
  // Serve SPIFFS files (CSS, JS, icons)
  _server.serveStatic("/style.css", SPIFFS, "/style.css");
  _server.serveStatic("/app.js",    SPIFFS, "/app.js");
  _server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");

  // Dashboard
  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    req->send(200, "text/html", _buildDashboard());
  });

  // Relay page
  _server.on("/relays", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    req->send(200, "text/html", _buildRelayPage());
  });

  // WiFi page
  _server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    req->send(200, "text/html", _buildWiFiPage());
  });

  // Config page
  _server.on("/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    req->send(200, "text/html", _buildConfigPage());
  });

  // Logs page
  _server.on("/logs", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    req->send(200, "text/html", _buildLogsPage());
  });
}

// ── API routes ────────────────────────────────────────────────
void TomatoWebServer::_setupAPIRoutes() {

  // GET /api/status — full system JSON
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    String json = "{";
    json += "\"sys\":" + SysMon.getStatsJSON() + ",";
    json += "\"wifi\":{"
            "\"ssid\":\"" + WifiMgr.getSSID() + "\","
            "\"ip\":\""   + WifiMgr.getIP()   + "\","
            "\"mac\":\""  + WifiMgr.getMACAddress() + "\","
            "\"rssi\":"   + WifiMgr.getRSSI() + ","
            "\"connected\":" + (WifiMgr.isConnected() ? "true" : "false") +
            "},";
    json += "\"relay\":" + Relays.getStatusJSON() + ",";
    json += "\"mqtt\":{\"connected\":" + String(MQTT.isConnected() ? "true" : "false") + "},";
    json += "\"version\":\"" TOMATO_VERSION "\","
            "\"buildDate\":\"" TOMATO_BUILD_DATE "\"";
    json += "}";
    _sendJSON(req, json);
  });

  // GET /api/stats — just system stats (lightweight, for polling)
  _server.on("/api/stats", HTTP_GET, [this](AsyncWebServerRequest* req) {
    _sendJSON(req, SysMon.getStatsJSON());
  });

  // GET /api/scan — WiFi scan
  _server.on("/api/scan", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    _sendJSON(req, WifiMgr.scanNetworksJSON());
  });

  // GET /api/logs
  _server.on("/api/logs", HTTP_GET, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    _sendJSON(req, SysMon.getLogJSON());
  });

  // POST /api/relay/1 and /api/relay/2
  _server.on("/api/relay/1", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    if (req->hasParam("state", true)) {
      String s = req->getParam("state", true)->value();
      Relays.setRelay1(s == "1" || s == "on");
      SysMon.addLog("Relay 1 → " + s);
      _sendJSON(req, Relays.getStatusJSON());
    } else _sendError(req, "missing state param");
  });

  _server.on("/api/relay/2", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    if (req->hasParam("state", true)) {
      String s = req->getParam("state", true)->value();
      Relays.setRelay2(s == "1" || s == "on");
      SysMon.addLog("Relay 2 → " + s);
      _sendJSON(req, Relays.getStatusJSON());
    } else _sendError(req, "missing state param");
  });

  // POST /api/relay/1/timer — ?state=on&delay=30000
  _server.on("/api/relay/1/timer", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    if (req->hasParam("state", true) && req->hasParam("delay", true)) {
      bool on = req->getParam("state", true)->value() == "on";
      unsigned long ms = req->getParam("delay", true)->value().toInt();
      Relays.timerRelay1(on, ms);
      _sendJSON(req, "{\"ok\":true}");
    } else _sendError(req, "missing params");
  });

  _server.on("/api/relay/2/timer", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    if (req->hasParam("state", true) && req->hasParam("delay", true)) {
      bool on = req->getParam("state", true)->value() == "on";
      unsigned long ms = req->getParam("delay", true)->value().toInt();
      Relays.timerRelay2(on, ms);
      _sendJSON(req, "{\"ok\":true}");
    } else _sendError(req, "missing params");
  });

  // POST /api/restart
  _server.on("/api/restart", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    SysMon.addLog("Restarting via web request");
    _sendJSON(req, "{\"ok\":true}");
    delay(500);
    ESP.restart();
  });

  // POST /api/factory-reset
  _server.on("/api/factory-reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    Storage.factoryReset();
    SysMon.addLog("Factory reset triggered via web");
    _sendJSON(req, "{\"ok\":true}");
    delay(1000);
    ESP.restart();
  });

  // GET /api/ping
  _server.on("/api/ping", HTTP_GET, [this](AsyncWebServerRequest* req) {
    _sendJSON(req, "{\"pong\":true,\"uptime\":" + String(millis()) + "}");
  });
}

// ── Config routes ─────────────────────────────────────────────
void TomatoWebServer::_setupConfigRoutes() {

  // POST /config/wifi
  _server.on("/config/wifi", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    if (req->hasParam("ssid", true)) {
      String ssid = req->getParam("ssid", true)->value();
      String pass = req->hasParam("pass", true) ? req->getParam("pass", true)->value() : "";
      WifiMgr.connect(ssid, pass);
      SysMon.addLog("WiFi config updated: " + ssid);
      _redirectTo(req, "/wifi");
    } else _sendError(req, "missing ssid");
  });

  // POST /config/device
  _server.on("/config/device", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    if (req->hasParam("name", true)) {
      Storage.setDeviceName(req->getParam("name", true)->value());
    }
    if (req->hasParam("tz", true)) {
      Storage.setTimezone(req->getParam("tz", true)->value());
    }
    if (req->hasParam("adminpass", true)) {
      Storage.setAdminPass(req->getParam("adminpass", true)->value());
    }
    SysMon.addLog("Device config updated");
    _redirectTo(req, "/config");
  });

  // POST /config/mqtt
  _server.on("/config/mqtt", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!_authenticate(req)) return;
    Storage.setMQTTEnabled(req->hasParam("enabled", true));
    if (req->hasParam("host", true))
      Storage.setMQTTHost(req->getParam("host", true)->value());
    if (req->hasParam("port", true))
      Storage.setMQTTPort(req->getParam("port", true)->value().toInt());
    if (req->hasParam("user", true))
      Storage.setMQTTUser(req->getParam("user", true)->value());
    if (req->hasParam("pass", true))
      Storage.setMQTTPass(req->getParam("pass", true)->value());
    SysMon.addLog("MQTT config updated");
    MQTT.begin(); // re-init with new settings
    _redirectTo(req, "/config");
  });
}

// ── WebSocket ─────────────────────────────────────────────────
void TomatoWebServer::_setupWSHandler() {
  _ws.onEvent(_onWSEvent);
}

void TomatoWebServer::_broadcastStats() {
  if (_ws.count() == 0) return;
  String json = "{\"type\":\"stats\","
                "\"data\":" + SysMon.getStatsJSON() + ","
                "\"rssi\":" + WifiMgr.getRSSI() + ","
                "\"internet\":" + (SysMon.getStats().internetOK ? "true" : "false") +
                "}";
  _ws.textAll(json);
}

void TomatoWebServer::_onWSEvent(AsyncWebSocket*, AsyncWebSocketClient* client,
                                  AwsEventType type, void*, uint8_t* data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[WS] Client #%u connected\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WS] Client #%u disconnected\n", client->id());
  } else if (type == WS_EVT_DATA) {
    String msg;
    for (size_t i = 0; i < len; i++) msg += (char)data[i];
    // Handle ping
    if (msg == "ping") client->text("{\"type\":\"pong\"}");
  }
}

// ================================================================
//  HTML Page Builders
// ================================================================

String TomatoWebServer::_buildDashboard() {
  const SysStats& s = SysMon.getStats();
  char uptime[32];
  uint32_t u = s.uptimeSeconds;
  snprintf(uptime, sizeof(uptime), "%02ud %02uh %02um",
    u/86400, (u%86400)/3600, (u%3600)/60);

  String html = HTML_HEAD;
  html += "<h1>Dashboard</h1>";
  html += "<div class='grid'>";

  // WiFi card
  html += "<div class='card'><h3>📡 WiFi</h3>";
  html += "<div class='stat'><span>Status</span><span class='" +
          String(WifiMgr.isConnected() ? "ok" : "err") + "'>" +
          (WifiMgr.isConnected() ? "Connected" : "Disconnected") + "</span></div>";
  html += "<div class='stat'><span>SSID</span><span>" + WifiMgr.getSSID() + "</span></div>";
  html += "<div class='stat'><span>IP</span><span>" + WifiMgr.getIP() + "</span></div>";
  html += "<div class='stat'><span>RSSI</span><span id='rssi'>" + String(WifiMgr.getRSSI()) + " dBm</span></div>";
  html += "<div class='stat'><span>MAC</span><span>" + WifiMgr.getMACAddress() + "</span></div>";
  html += "</div>";

  // System card
  html += "<div class='card'><h3>⚡ System</h3>";
  html += "<div class='stat'><span>Uptime</span><span>" + String(uptime) + "</span></div>";
  html += "<div class='stat'><span>Free Heap</span><span id='heap'>" + String(s.freeHeap/1024) + " KB</span></div>";
  html += "<div class='stat'><span>Heap Used</span><span id='heapPct'>" + String(s.heapUsedPct) + "%</span></div>";
  html += "<div class='stat'><span>CPU Temp</span><span id='temp'>" + String(s.cpuTempC, 1) + " °C</span></div>";
  html += "<div class='stat'><span>Chip</span><span>" + s.chipModel + " @ " + String(s.cpuFreqMhz) + "MHz</span></div>";
  html += "<div class='stat'><span>Flash</span><span>" + String(s.flashSizeMB) + " MB</span></div>";
  html += "</div>";

  // Network card
  html += "<div class='card'><h3>🌐 Network</h3>";
  html += "<div class='stat'><span>Internet</span><span id='inet' class='" +
          String(s.internetOK ? "ok" : "err") + "'>" +
          (s.internetOK ? "Online" : "Offline") + "</span></div>";
  html += "<div class='stat'><span>Ping</span><span id='ping'>" +
          (s.pingMs >= 0 ? String(s.pingMs, 1) + " ms" : "—") + "</span></div>";
  html += "<div class='stat'><span>MQTT</span><span class='" +
          String(MQTT.isConnected() ? "ok" : "warn") + "'>" +
          (MQTT.isConnected() ? "Connected" : "Disconnected") + "</span></div>";
  html += "</div>";

  // Relay card
  html += "<div class='card'><h3>🔌 Relays</h3>";
  html += "<div class='relay-row'>";
  html += "<div><b>Relay 1</b><br><span class='" +
          String(Relays.getRelay1State() ? "ok" : "off") + "'>" +
          (Relays.getRelay1State() ? "ON" : "OFF") + "</span></div>";
  html += "<div><b>Relay 2</b><br><span class='" +
          String(Relays.getRelay2State() ? "ok" : "off") + "'>" +
          (Relays.getRelay2State() ? "ON" : "OFF") + "</span></div>";
  html += "</div>";
  html += "<a href='/relays' class='btn'>Manage Relays</a>";
  html += "</div>";

  html += "</div>"; // end grid

  // Actions
  html += "<div class='actions'>";
  html += "<form action='/api/restart' method='POST' onsubmit=\"return confirm('Restart device?')\">"
          "<button class='btn warn'>🔄 Restart</button></form>";
  html += "<form action='/api/factory-reset' method='POST' onsubmit=\"return confirm('Factory reset? All settings will be lost!')\">"
          "<button class='btn err'>⚠️ Factory Reset</button></form>";
  html += "</div>";

  html += HTML_FOOT;
  return html;
}

String TomatoWebServer::_buildRelayPage() {
  String html = HTML_HEAD;
  html += "<h1>🔌 Relay Control</h1>";

  for (int r = 1; r <= 2; r++) {
    bool state = (r == 1) ? Relays.getRelay1State() : Relays.getRelay2State();
    html += "<div class='card'><h3>Relay " + String(r) + "</h3>";
    html += "<div class='stat'><span>State</span><span class='" +
            String(state ? "ok" : "off") + "'>" +
            (state ? "ON" : "OFF") + "</span></div>";

    html += "<div class='btn-group'>";
    html += "<form action='/api/relay/" + String(r) + "' method='POST' style='display:inline'>"
            "<input type='hidden' name='state' value='on'>"
            "<button class='btn ok'>Turn ON</button></form> ";
    html += "<form action='/api/relay/" + String(r) + "' method='POST' style='display:inline'>"
            "<input type='hidden' name='state' value='off'>"
            "<button class='btn err'>Turn OFF</button></form>";
    html += "</div>";

    html += "<h4>⏱ Set Timer</h4>";
    html += "<form action='/api/relay/" + String(r) + "/timer' method='POST' class='timer-form'>"
            "<select name='state'><option value='on'>Turn ON</option><option value='off'>Turn OFF</option></select>"
            "<input type='number' name='delay' placeholder='Delay (ms)' min='1000' value='5000'>"
            "<button class='btn'>Set Timer</button></form>";
    html += "</div>";
  }

  html += HTML_FOOT;
  return html;
}

String TomatoWebServer::_buildWiFiPage() {
  String html = HTML_HEAD;
  html += "<h1>📡 WiFi Settings</h1>";

  html += "<div class='card'><h3>Current Connection</h3>";
  html += "<div class='stat'><span>SSID</span><span>" + WifiMgr.getSSID() + "</span></div>";
  html += "<div class='stat'><span>IP</span><span>" + WifiMgr.getIP() + "</span></div>";
  html += "<div class='stat'><span>RSSI</span><span>" + String(WifiMgr.getRSSI()) + " dBm</span></div>";
  html += "</div>";

  html += "<div class='card'><h3>Connect to Network</h3>";
  html += "<form action='/config/wifi' method='POST'>";
  html += "<label>SSID<input type='text' name='ssid' placeholder='WiFi network name' required></label>";
  html += "<label>Password<input type='password' name='pass' placeholder='Leave blank for open network'></label>";
  html += "<button class='btn' type='submit'>Connect</button>";
  html += "</form></div>";

  html += "<div class='card'><h3>Scan Networks</h3>";
  html += "<button class='btn' onclick='scanWifi()'>Scan</button>";
  html += "<div id='scan-results'></div>";
  html += "</div>";

  html += HTML_FOOT;
  return html;
}

String TomatoWebServer::_buildConfigPage() {
  String html = HTML_HEAD;
  html += "<h1>⚙️ Configuration</h1>";

  // Device config
  html += "<div class='card'><h3>Device</h3>";
  html += "<form action='/config/device' method='POST'>";
  html += "<label>Device Name<input type='text' name='name' value='" + Storage.getDeviceName() + "'></label>";
  html += "<label>Timezone<input type='text' name='tz' value='" + Storage.getTimezone() + "' placeholder='e.g. EST5EDT,M3.2.0,M11.1.0'></label>";
  html += "<label>Admin Password<input type='password' name='adminpass' placeholder='New password'></label>";
  html += "<label>Version<input type='text' value='" TOMATO_VERSION "' readonly></label>";
  html += "<button class='btn' type='submit'>Save Device Settings</button>";
  html += "</form></div>";

  // MQTT config
  html += "<div class='card'><h3>MQTT</h3>";
  html += "<form action='/config/mqtt' method='POST'>";
  html += "<label class='checkbox'><input type='checkbox' name='enabled'" +
          String(Storage.getMQTTEnabled() ? " checked" : "") + "> Enable MQTT</label>";
  html += "<label>Broker Host<input type='text' name='host' value='" + Storage.getMQTTHost() + "' placeholder='192.168.1.100'></label>";
  html += "<label>Port<input type='number' name='port' value='" + String(Storage.getMQTTPort()) + "'></label>";
  html += "<label>Username<input type='text' name='user' value='" + Storage.getMQTTUser() + "'></label>";
  html += "<label>Password<input type='password' name='pass' placeholder='MQTT password'></label>";
  html += "<button class='btn' type='submit'>Save MQTT Settings</button>";
  html += "</form></div>";

  html += HTML_FOOT;
  return html;
}

String TomatoWebServer::_buildLogsPage() {
  String html = HTML_HEAD;
  html += "<h1>📋 System Logs</h1>";
  html += "<div class='card'>";
  html += "<div class='log-box' id='log-entries'>";
  html += "<p class='dim'>Loading logs...</p>";
  html += "</div>";
  html += "<button class='btn' onclick='loadLogs()'>Refresh</button>";
  html += "</div>";
  html += "<script>loadLogs();</script>";
  html += HTML_FOOT;
  return html;
}
