#include "ota_manager.h"
#include "led_manager.h"
#include "system_monitor.h"

OTAManager OTA;

OTAManager::OTAManager() : _updating(false) {}

void OTAManager::begin(const String& hostname, const String& password) {
#if !FEATURE_OTA
  Serial.println("[OTA] Feature disabled");
  return;
#endif
  _setup(hostname, password);
  Serial.printf("[OTA] Ready — hostname: %s\n", hostname.c_str());
}

void OTAManager::loop() {
#if FEATURE_OTA
  ArduinoOTA.handle();
#endif
}

void OTAManager::_setup(const String& hostname, const String& password) {
  ArduinoOTA.setHostname(hostname.c_str());
  ArduinoOTA.setPassword(password.c_str());
  ArduinoOTA.setPort(3232);

  ArduinoOTA.onStart([this]() {
    _updating = true;
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
    Serial.println("[OTA] Start updating " + type);
    SysMon.addLog("OTA update started: " + type);
    LED.setPattern(LEDPattern::BLINK_FAST);
  });

  ArduinoOTA.onEnd([this]() {
    _updating = false;
    Serial.println("\n[OTA] Update complete — rebooting");
    SysMon.addLog("OTA update complete");
    LED.setPattern(LEDPattern::ON);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([this](ota_error_t error) {
    _updating = false;
    const char* msg = "Unknown error";
    if      (error == OTA_AUTH_ERROR)    msg = "Auth failed";
    else if (error == OTA_BEGIN_ERROR)   msg = "Begin failed";
    else if (error == OTA_CONNECT_ERROR) msg = "Connect failed";
    else if (error == OTA_RECEIVE_ERROR) msg = "Receive failed";
    else if (error == OTA_END_ERROR)     msg = "End failed";
    Serial.printf("[OTA] Error[%u]: %s\n", error, msg);
    SysMon.addLog(String("OTA error: ") + msg);
    LED.setPattern(LEDPattern::BLINK_SLOW);
  });

  ArduinoOTA.begin();
}
