#pragma once

// ================================================================
//  ESP32 Tomato — Central Configuration
// ================================================================

// ── Firmware info ────────────────────────────────────────────────
#define TOMATO_VERSION       "1.0.0"
#define TOMATO_NAME          "ESP32 Tomato"
#define TOMATO_BUILD_DATE    __DATE__

// ── WiFi ─────────────────────────────────────────────────────────
#define WIFI_AP_SSID         "ESP32-Tomato"
#define WIFI_AP_PASSWORD     "tomato123"      // Min 8 chars for WPA2
#define WIFI_AP_IP           IPAddress(192,168,4,1)
#define WIFI_AP_GATEWAY      IPAddress(192,168,4,1)
#define WIFI_AP_SUBNET       IPAddress(255,255,255,0)
#define WIFI_CONNECT_TIMEOUT 20000            // ms
#define WIFI_RETRY_INTERVAL  30000            // ms

// ── Pins ─────────────────────────────────────────────────────────
#define PIN_LED_STATUS       2               // Onboard LED (active LOW on most ESP32)
#define PIN_BTN_RESET        0               // BOOT button = factory reset
#define PIN_BTN_RESET_HOLD   3000            // ms hold to factory reset
#define PIN_DHT              4               // DHT22 temperature sensor (optional)
#define PIN_RELAY_1          26              // Relay 1
#define PIN_RELAY_2          27              // Relay 2
#define PIN_ADC_BATTERY      34             // Battery voltage ADC (if applicable)

// ── Web server ───────────────────────────────────────────────────
#define WEB_PORT             80
#define WEB_AUTH_USER        "admin"
#define WEB_AUTH_PASS        "tomato"        // Change in setup!
#define WEB_SESSION_TIMEOUT  1800            // seconds

// ── MQTT (optional) ──────────────────────────────────────────────
#define MQTT_PORT            1883
#define MQTT_KEEPALIVE       60
#define MQTT_TOPIC_BASE      "esp32tomato"
#define MQTT_RECONNECT_MS    5000

// ── NTP ──────────────────────────────────────────────────────────
#define NTP_SERVER_1         "pool.ntp.org"
#define NTP_SERVER_2         "time.google.com"
#define NTP_TIMEZONE         "UTC0"          // Change to your timezone
                                             // e.g. "EST5EDT,M3.2.0,M11.1.0"

// ── Storage (Preferences namespace keys) ─────────────────────────
#define PREF_NAMESPACE       "tomato"
#define PREF_WIFI_SSID       "wifi_ssid"
#define PREF_WIFI_PASS       "wifi_pass"
#define PREF_DEVICE_NAME     "dev_name"
#define PREF_MQTT_HOST       "mqtt_host"
#define PREF_MQTT_PORT       "mqtt_port"
#define PREF_MQTT_USER       "mqtt_user"
#define PREF_MQTT_PASS       "mqtt_pass"
#define PREF_MQTT_EN         "mqtt_en"
#define PREF_RELAY1_STATE    "relay1"
#define PREF_RELAY2_STATE    "relay2"
#define PREF_TIMEZONE        "timezone"
#define PREF_ADMIN_PASS      "admin_pass"
#define PREF_BOOT_COUNT      "boot_cnt"

// ── Monitoring intervals ──────────────────────────────────────────
#define INTERVAL_SENSOR_MS   5000           // Read temp/humidity every 5s
#define INTERVAL_PING_MS     10000          // Ping gateway every 10s
#define INTERVAL_UPLOAD_MS   30000          // Upload stats to MQTT every 30s
#define INTERVAL_NTP_MS      3600000        // Re-sync NTP every hour

// ── System limits ─────────────────────────────────────────────────
#define MAX_WIFI_CLIENTS     4
#define MAX_LOG_ENTRIES      50
#define PING_HOST            "8.8.8.8"
#define PING_TIMEOUT_MS      1000

// ── Feature flags ─────────────────────────────────────────────────
#define FEATURE_DHT          false           // Set true if DHT22 wired up
#define FEATURE_RELAY        false           // Set true if relays wired up
#define FEATURE_MQTT         true
#define FEATURE_OTA          true
#define FEATURE_PING         true
#define FEATURE_SYSLOG       true

// ── Debug ─────────────────────────────────────────────────────────
#ifdef TOMATO_DEBUG
  #define DBG(x)   Serial.print(x)
  #define DBGLN(x) Serial.println(x)
  #define DBGF(...)Serial.printf(__VA_ARGS__)
#else
  #define DBG(x)
  #define DBGLN(x)
  #define DBGF(...)
#endif
