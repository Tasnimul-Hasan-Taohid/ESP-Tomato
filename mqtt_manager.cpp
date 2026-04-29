#include "mqtt_manager.h"
#include "storage.h"
#include "system_monitor.h"
#include "relay_manager.h"
#include "wifi_manager.h"

MQTTManager* MQTTManager::_instance = nullptr;
MQTTManager  MQTT;

MQTTManager::MQTTManager()
  : _client(_wifiClient), _lastReconnect(0), _lastPublish(0) {
  _instance = this;
}

void MQTTManager::begin() {
#if !FEATURE_MQTT
  Serial.println("[MQTT] Feature disabled");
  return;
#endif
  if (!Storage.getMQTTEnabled()) {
    Serial.println("[MQTT] Disabled in settings");
    return;
  }

  String host = Storage.getMQTTHost();
  if (host.length() == 0) {
    Serial.println("[MQTT] No broker configured");
    return;
  }

  _client.setServer(host.c_str(), Storage.getMQTTPort());
  _client.setCallback(_staticCallback);
  _client.setKeepAlive(MQTT_KEEPALIVE);
  Serial.printf("[MQTT] Configured: %s:%u\n", host.c_str(), Storage.getMQTTPort());
}

void MQTTManager::loop() {
#if !FEATURE_MQTT
  return;
#endif
  if (!Storage.getMQTTEnabled()) return;
  if (!WifiMgr.isConnected()) return;

  if (!_client.connected()) {
    if (millis() - _lastReconnect >= MQTT_RECONNECT_MS) {
      _lastReconnect = millis();
      _reconnect();
    }
    return;
  }

  _client.loop();

  // Periodic stats publish
  if (millis() - _lastPublish >= INTERVAL_UPLOAD_MS) {
    _lastPublish = millis();
    publishStats();
  }
}

bool MQTTManager::isConnected() const {
  return _client.connected();
}

void MQTTManager::publish(const String& subtopic, const String& payload, bool retained) {
  if (!_client.connected()) return;
  String topic = String(MQTT_TOPIC_BASE) + "/" +
                 Storage.getDeviceName() + "/" + subtopic;
  _client.publish(topic.c_str(), payload.c_str(), retained);
  DBGF("[MQTT] PUB %s = %s\n", topic.c_str(), payload.c_str());
}

void MQTTManager::publishStats() {
  const SysStats& s = SysMon.getStats();
  publish("heap/free",    String(s.freeHeap));
  publish("heap/pct",     String(s.heapUsedPct));
  publish("temp",         String(s.cpuTempC, 1));
  publish("uptime",       String(s.uptimeSeconds));
  publish("rssi",         String(WifiMgr.getRSSI()));
  publish("ping",         String(s.pingMs, 1));
  publish("internet",     s.internetOK ? "1" : "0");
  publish("relay/1",      Relays.getRelay1State() ? "1" : "0", true);
  publish("relay/2",      Relays.getRelay2State() ? "1" : "0", true);
  Serial.println("[MQTT] Stats published");
}

bool MQTTManager::_reconnect() {
  String clientId = Storage.getDeviceName() + "_" + String(random(0xFFFF), HEX);
  String user     = Storage.getMQTTUser();
  String pass     = Storage.getMQTTPass();

  bool ok;
  if (user.length() > 0) {
    ok = _client.connect(clientId.c_str(), user.c_str(), pass.c_str());
  } else {
    ok = _client.connect(clientId.c_str());
  }

  if (ok) {
    Serial.println("[MQTT] Connected to broker");
    // Subscribe to command topics
    String base = String(MQTT_TOPIC_BASE) + "/" + Storage.getDeviceName();
    _client.subscribe((base + "/relay/1/set").c_str());
    _client.subscribe((base + "/relay/2/set").c_str());
    _client.subscribe((base + "/restart").c_str());
    publish("status", "online", true);
  } else {
    Serial.printf("[MQTT] Connect failed rc=%d\n", _client.state());
  }
  return ok;
}

void MQTTManager::_staticCallback(char* topic, byte* payload, unsigned int len) {
  if (!_instance) return;
  String t(topic);
  String p;
  for (unsigned int i = 0; i < len; i++) p += (char)payload[i];

  DBGF("[MQTT] SUB %s = %s\n", t.c_str(), p.c_str());

  String base = String(MQTT_TOPIC_BASE) + "/" + Storage.getDeviceName();

  if (t == base + "/relay/1/set") {
    Relays.setRelay1(p == "1" || p == "on" || p == "ON");
  } else if (t == base + "/relay/2/set") {
    Relays.setRelay2(p == "1" || p == "on" || p == "ON");
  } else if (t == base + "/restart") {
    Serial.println("[MQTT] Remote restart command received");
    delay(500); ESP.restart();
  }

  if (_instance->_msgCallback) {
    _instance->_msgCallback(t, p);
  }
}
