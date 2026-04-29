#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "config.h"

// ================================================================
//  WebServer — serves dashboard, REST API, config portal, OTA UI
// ================================================================
class TomatoWebServer {
public:
  TomatoWebServer();
  void begin();
  void loop();   // Minimal — async server handles itself

private:
  AsyncWebServer _server;
  AsyncWebSocket _ws;

  // ── Route setup ──────────────────────────────────────────────
  void _setupStaticRoutes();
  void _setupAPIRoutes();
  void _setupConfigRoutes();
  void _setupWSHandler();

  // ── Middleware ───────────────────────────────────────────────
  bool _authenticate(AsyncWebServerRequest* req);
  void _sendJSON(AsyncWebServerRequest* req, const String& json, int code = 200);
  void _sendError(AsyncWebServerRequest* req, const String& msg, int code = 400);
  void _redirectTo(AsyncWebServerRequest* req, const String& url);

  // ── Page builders ────────────────────────────────────────────
  String _buildDashboard();
  String _buildConfigPage();
  String _buildRelayPage();
  String _buildLogsPage();
  String _buildWiFiPage();

  // ── WebSocket ────────────────────────────────────────────────
  void _broadcastStats();
  unsigned long _lastBroadcast;

  static void _onWSEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                         AwsEventType type, void* arg,
                         uint8_t* data, size_t len);
};

extern TomatoWebServer WebServer;
