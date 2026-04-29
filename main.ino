// ================================================================
//  ESP32 Tomato — Main Entry Point
//  v1.0.0
//
//  A full-featured ESP32 IoT platform with:
//    - WiFi management (STA + AP fallback)
//    - Web dashboard with WebSocket live updates
//    - Dual relay control with timers
//    - MQTT publishing and remote control
//    - OTA firmware updates
//    - System monitoring (heap, temp, ping, uptime)
//    - LED status indicator
//    - Non-blocking architecture throughout
// ================================================================

#include <Arduino.h>
#include <SPIFFS.h>
#include <time.h>

#include "config.h"
#include "storage.h"
#include "wifi_manager.h"
#include "system_monitor.h"
#include "relay_manager.h"
#include "mqtt_manager.h"
#include "ota_manager.h"
#include "led_manager.h"
#include "web_server.h"

// ── Button state (factory reset) ─────────────────────────────
static bool          btnLastState  = HIGH;
static unsigned long btnPressTime  = 0;
static bool          btnHoldFired  = false;

// ── NTP ──────────────────────────────────────────────────────
static unsigned long lastNTPSync = 0;
static bool          ntpSynced   = false;

// ── Forward declarations ──────────────────────────────────────
void checkResetButton();
void syncNTP();
void updateLEDFromState();

// ================================================================
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("\n");
  Serial.println("  ████████╗ ██████╗ ███╗   ███╗ █████╗ ████████╗ ██████╗ ");
  Serial.println("     ██╔══╝██╔═══██╗████╗ ████║██╔══██╗╚══██╔══╝██╔═══██╗");
  Serial.println("     ██║   ██║   ██║██╔████╔██║███████║   ██║   ██║   ██║");
  Serial.println("     ██║   ██║   ██║██║╚██╔╝██║██╔══██║   ██║   ██║   ██║");
  Serial.println("     ██║   ╚██████╔╝██║ ╚═╝ ██║██║  ██║   ██║   ╚██████╔╝");
  Serial.println("     ╚═╝    ╚═════╝ ╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ");
  Serial.println();
  Serial.printf("  🍅 ESP32 Tomato  v%s  |  Built: %s\n\n", TOMATO_VERSION, TOMATO_BUILD_DATE);

  // ── 1. Storage ──────────────────────────────────────────────
  Storage.begin();

  // ── 2. LED ──────────────────────────────────────────────────
  LED.begin();
  LED.setPattern(LEDPattern::BLINK_FAST); // Fast blink during boot

  // ── 3. Factory reset button ─────────────────────────────────
  pinMode(PIN_BTN_RESET, INPUT_PULLUP);

  // ── 4. SPIFFS ───────────────────────────────────────────────
  if (!SPIFFS.begin(true)) {
    Serial.println("[MAIN] SPIFFS mount failed — web UI may not load");
    Serial.println("[MAIN] Run 'pio run --target uploadfs' to upload web files");
  } else {
    Serial.println("[MAIN] SPIFFS mounted OK");
  }

  // ── 5. System Monitor ───────────────────────────────────────
  SysMon.begin();

  // ── 6. Relay Manager ────────────────────────────────────────
  Relays.begin();

  // ── 7. WiFi ─────────────────────────────────────────────────
  LED.setPattern(LEDPattern::BLINK_SLOW); // Slow blink = connecting
  WifiMgr.begin();

  // ── 8. Wait for initial WiFi (non-blocking poll, max 20s) ───
  unsigned long wifiWait = millis();
  while (!WifiMgr.isConnected() && !WifiMgr.isAPMode() &&
         millis() - wifiWait < WIFI_CONNECT_TIMEOUT) {
    WifiMgr.loop();
    LED.loop();
    delay(50);
  }

  // ── 9. NTP (only in STA mode) ───────────────────────────────
  if (WifiMgr.isConnected()) {
    syncNTP();
  }

  // ── 10. Web Server ──────────────────────────────────────────
  WebServer.begin();

  // ── 11. MQTT ────────────────────────────────────────────────
  MQTT.begin();

  // Register MQTT message handler (custom app logic)
  MQTT.onMessage([](const String& topic, const String& payload) {
    SysMon.addLog("MQTT msg: " + topic + " = " + payload);
  });

  // ── 12. OTA ─────────────────────────────────────────────────
  if (WifiMgr.isConnected()) {
    OTA.begin(Storage.getDeviceName(), Storage.getAdminPass());
  }

  // ── 13. Final LED state ─────────────────────────────────────
  updateLEDFromState();

  SysMon.addLog("Boot complete — v" TOMATO_VERSION);
  Serial.printf("[MAIN] Boot #%u complete\n", Storage.getBootCount());
  Serial.printf("[MAIN] Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("[MAIN] Dashboard: http://%s\n", WifiMgr.getIP().c_str());
}

// ================================================================
void loop() {
  // Non-blocking — all managers use millis() internally

  // Core managers
  WifiMgr.loop();
  SysMon.loop();
  Relays.loop();
  MQTT.loop();
  LED.loop();
  WebServer.loop();

  // OTA only when WiFi connected
  if (WifiMgr.isConnected()) {
    OTA.loop();
  }

  // Factory reset button check
  checkResetButton();

  // NTP re-sync every hour
  if (WifiMgr.isConnected() && millis() - lastNTPSync >= INTERVAL_NTP_MS) {
    syncNTP();
  }

  // LED pattern based on current state
  static unsigned long lastLEDUpdate = 0;
  if (millis() - lastLEDUpdate >= 2000) {
    lastLEDUpdate = millis();
    updateLEDFromState();
  }

  // Yield to allow background tasks (WiFi stack, etc.)
  yield();
}

// ================================================================
//  Factory Reset Button
//  Hold BOOT button for PIN_BTN_RESET_HOLD ms to factory reset
// ================================================================
void checkResetButton() {
  bool btnState = digitalRead(PIN_BTN_RESET);

  if (btnState == LOW && btnLastState == HIGH) {
    // Falling edge — button just pressed
    btnPressTime = millis();
    btnHoldFired = false;
  }

  if (btnState == LOW && !btnHoldFired) {
    unsigned long held = millis() - btnPressTime;
    if (held >= PIN_BTN_RESET_HOLD) {
      btnHoldFired = true;
      Serial.println("[MAIN] BOOT button held — factory reset!");
      SysMon.addLog("Factory reset triggered via button");
      LED.setPattern(LEDPattern::BLINK_FAST);
      delay(500);
      Storage.factoryReset();
      delay(500);
      ESP.restart();
    }
  }

  btnLastState = btnState;
}

// ================================================================
//  NTP Sync
// ================================================================
void syncNTP() {
  Serial.println("[NTP] Syncing time...");
  configTzTime(Storage.getTimezone().c_str(), NTP_SERVER_1, NTP_SERVER_2);

  // Wait up to 5 seconds for sync
  unsigned long start = millis();
  while (time(nullptr) < 1000000000 && millis() - start < 5000) {
    delay(100);
  }

  time_t now = time(nullptr);
  if (now > 1000000000) {
    ntpSynced    = true;
    lastNTPSync  = millis();
    struct tm ti;
    localtime_r(&now, &ti);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
    Serial.printf("[NTP] Synced: %s\n", buf);
    SysMon.addLog(String("NTP synced: ") + buf);
  } else {
    Serial.println("[NTP] Sync failed");
    lastNTPSync = millis(); // Try again next interval
  }
}

// ================================================================
//  LED Pattern — reflects current system state
// ================================================================
void updateLEDFromState() {
  if (OTA.isUpdating()) {
    LED.setPattern(LEDPattern::BLINK_FAST);
    return;
  }
  if (WifiMgr.isConnected()) {
    LED.setPattern(LEDPattern::HEARTBEAT);
  } else if (WifiMgr.isAPMode()) {
    LED.setPattern(LEDPattern::BLINK_SLOW);
  } else {
    LED.setPattern(LEDPattern::BLINK_FAST);
  }
}
