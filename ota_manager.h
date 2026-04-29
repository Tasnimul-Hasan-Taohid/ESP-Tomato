#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"

// ================================================================
//  OTAManager — ArduinoOTA wrapper with progress callbacks
// ================================================================
class OTAManager {
public:
  OTAManager();
  void begin(const String& hostname, const String& password);
  void loop();
  bool isUpdating() const { return _updating; }

private:
  bool _updating;
  void _setup(const String& hostname, const String& password);
};

extern OTAManager OTA;
