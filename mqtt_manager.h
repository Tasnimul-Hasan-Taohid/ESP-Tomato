#pragma once
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "config.h"

// ================================================================
//  MQTTManager — publish stats, subscribe to relay commands
// ================================================================
class MQTTManager {
public:
  MQTTManager();
  void begin();
  void loop();

  bool isConnected() const;
  void publish(const String& subtopic, const String& payload, bool retained = false);
  void publishStats();

  // Callback type for incoming messages
  using MessageCallback = std::function<void(const String& topic, const String& payload)>;
  void onMessage(MessageCallback cb) { _msgCallback = cb; }

private:
  WiFiClient      _wifiClient;
  PubSubClient    _client;
  unsigned long   _lastReconnect;
  unsigned long   _lastPublish;
  MessageCallback _msgCallback;

  bool _reconnect();
  static void _staticCallback(char* topic, byte* payload, unsigned int len);
  static MQTTManager* _instance;
};

extern MQTTManager MQTT;
