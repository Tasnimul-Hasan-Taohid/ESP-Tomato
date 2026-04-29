#include "system_monitor.h"
#include "wifi_manager.h"
#include <esp_system.h>

#if FEATURE_PING
  #include <ESP32Ping.h>
#endif

SystemMonitor SysMon;

SystemMonitor::SystemMonitor()
  : _lastSensorRead(0), _lastPing(0),
    _lastLoopCount(0), _loopCount(0), _lastHz(0),
    _logHead(0), _logCount(0)
{
  memset(&_stats, 0, sizeof(_stats));
  _stats.pingMs = -1;
}

void SystemMonitor::begin() {
  _stats.chipModel   = ESP.getChipModel();
  _stats.cpuFreqMhz  = getCpuFrequencyMhz();
  _stats.flashSizeMB = ESP.getFlashChipSize() / (1024 * 1024);
  _stats.sdkVersion  = ESP.getSdkVersion();

  addLog("System monitor started");
  addLog("Chip: " + _stats.chipModel);
  addLog("CPU: " + String(_stats.cpuFreqMhz) + " MHz");
  addLog("Flash: " + String(_stats.flashSizeMB) + " MB");

  Serial.printf("[SYS] Chip: %s | CPU: %u MHz | Flash: %u MB\n",
    _stats.chipModel.c_str(), _stats.cpuFreqMhz, _stats.flashSizeMB);
}

void SystemMonitor::loop() {
  _loopCount++;
  unsigned long now = millis();

  // Update Hz every second
  if (now - _lastHz >= 1000) {
    _stats.loopHz  = _loopCount - _lastLoopCount;
    _lastLoopCount = _loopCount;
    _lastHz        = now;
  }

  if (now - _lastSensorRead >= INTERVAL_SENSOR_MS) {
    _lastSensorRead = now;
    _readSensors();
  }

  if (now - _lastPing >= INTERVAL_PING_MS) {
    _lastPing = now;
    _doPing();
  }
}

void SystemMonitor::_readSensors() {
  _stats.freeHeap    = ESP.getFreeHeap();
  _stats.totalHeap   = ESP.getHeapSize();
  _stats.heapUsedPct = 100 - ((_stats.freeHeap * 100) / _stats.totalHeap);
  _stats.uptimeSeconds = millis() / 1000;

  // ESP32 has an internal temperature sensor (rough approximation)
  // Returns values in Fahrenheit internally; convert to Celsius
  // Note: this is approximate (±5°C accuracy) and reflects die temp
  extern uint8_t temprature_sens_read();
  _stats.cpuTempC = (temprature_sens_read() - 32) / 1.8f;

  DBGF("[SYS] Heap: %u/%u (%u%%)  Temp: %.1f°C\n",
    _stats.freeHeap, _stats.totalHeap, _stats.heapUsedPct, _stats.cpuTempC);
}

void SystemMonitor::_doPing() {
#if FEATURE_PING
  if (!WifiMgr.isConnected()) {
    _stats.pingMs    = -1;
    _stats.internetOK = false;
    return;
  }
  bool ok = Ping.ping(PING_HOST, 1);
  if (ok) {
    _stats.pingMs     = Ping.averageTime();
    _stats.internetOK = true;
  } else {
    _stats.pingMs     = -1;
    _stats.internetOK = false;
  }
  DBGF("[SYS] Ping %s: %.1f ms  ok=%d\n", PING_HOST, _stats.pingMs, ok);
#endif
}

String SystemMonitor::getStatsJSON() const {
  char buf[512];
  snprintf(buf, sizeof(buf),
    "{"
    "\"freeHeap\":%u,"
    "\"totalHeap\":%u,"
    "\"heapUsedPct\":%u,"
    "\"cpuTempC\":%.1f,"
    "\"uptimeSec\":%u,"
    "\"pingMs\":%.1f,"
    "\"internetOK\":%s,"
    "\"loopHz\":%u,"
    "\"chipModel\":\"%s\","
    "\"cpuFreqMhz\":%u,"
    "\"flashSizeMB\":%u,"
    "\"sdkVersion\":\"%s\","
    "\"rssi\":%d"
    "}",
    _stats.freeHeap,
    _stats.totalHeap,
    _stats.heapUsedPct,
    _stats.cpuTempC,
    _stats.uptimeSeconds,
    _stats.pingMs,
    _stats.internetOK ? "true" : "false",
    _stats.loopHz,
    _stats.chipModel.c_str(),
    _stats.cpuFreqMhz,
    _stats.flashSizeMB,
    _stats.sdkVersion.c_str(),
    WifiMgr.getRSSI()
  );
  return String(buf);
}

void SystemMonitor::addLog(const String& msg) {
  char ts[32];
  unsigned long s = millis() / 1000;
  snprintf(ts, sizeof(ts), "[%02lu:%02lu:%02lu] ",
    s / 3600, (s % 3600) / 60, s % 60);
  _log[_logHead] = String(ts) + msg;
  _logHead = (_logHead + 1) % MAX_LOG_ENTRIES;
  if (_logCount < MAX_LOG_ENTRIES) _logCount++;
  Serial.println("[LOG] " + msg);
}

String SystemMonitor::getLogJSON() const {
  String json = "[";
  int start = (_logCount < MAX_LOG_ENTRIES) ? 0 : _logHead;
  for (int i = 0; i < _logCount; i++) {
    int idx = (start + i) % MAX_LOG_ENTRIES;
    if (i > 0) json += ",";
    json += "\"" + _log[idx] + "\"";
  }
  json += "]";
  return json;
}
