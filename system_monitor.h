#pragma once
#include <Arduino.h>
#include "config.h"

// ================================================================
//  SystemMonitor — collects CPU, RAM, uptime, ping, chip info
// ================================================================
struct SysStats {
  uint32_t freeHeap;
  uint32_t totalHeap;
  uint32_t heapUsedPct;
  float    cpuTempC;        // ESP32 internal sensor (approx)
  uint32_t uptimeSeconds;
  float    pingMs;          // -1 = failed
  uint32_t loopHz;          // Approx loops per second
  bool     internetOK;
  String   chipModel;
  uint32_t cpuFreqMhz;
  uint32_t flashSizeMB;
  String   sdkVersion;
};

class SystemMonitor {
public:
  SystemMonitor();
  void begin();
  void loop();

  const SysStats& getStats() const { return _stats; }
  String          getStatsJSON() const;
  String          getLogJSON() const;
  void            addLog(const String& msg);

private:
  SysStats      _stats;
  unsigned long _lastSensorRead;
  unsigned long _lastPing;
  unsigned long _lastLoopCount;
  unsigned long _loopCount;
  unsigned long _lastHz;

  // Circular log buffer
  String        _log[MAX_LOG_ENTRIES];
  int           _logHead;
  int           _logCount;

  void _readSensors();
  void _doPing();
  void _updateHz();
};

extern SystemMonitor SysMon;
