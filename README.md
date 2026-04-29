# 🍅 ESP32 Tomato

A full-featured ESP32 IoT platform with a web dashboard, dual relay control, MQTT integration, OTA firmware updates, and live system monitoring. Runs on any standard ESP32 development board with no extra hardware required — everything optional features are clearly labelled in `config.h`.

The name is a nod to Tomato firmware — the idea that a small, capable device should do a lot without getting in the way.

---

## Features

| Feature | Details |
|---|---|
| 📡 WiFi Manager | STA mode with AP fallback, auto-reconnect |
| 🌐 Web Dashboard | Async HTTP server with real-time WebSocket updates |
| 🔌 Dual Relay Control | Toggle, timed, and MQTT-controlled relays |
| 📊 System Monitor | Heap, CPU temp, ping, uptime, loop frequency |
| 📡 MQTT Client | Publishes stats, subscribes to relay commands |
| ✈️ OTA Updates | Firmware updates over WiFi from Arduino IDE or PlatformIO |
| 💡 LED Status | Non-blocking patterns: heartbeat, blink, pulse |
| 🔄 Auto-Reconnect | Retries WiFi every 30 seconds if connection drops |
| 🕐 NTP Time | Syncs system clock with configurable timezone |
| 💾 Persistent Storage | All settings survive reboots via ESP32 Preferences |
| 🔘 Factory Reset | Hold BOOT button for 3 seconds to wipe all settings |
| 📋 System Log | Circular in-memory log, viewable from dashboard |

---

## Hardware Requirements

| Component | Details |
|---|---|
| **ESP32 Dev Board** | Any standard 30-pin or 38-pin ESP32 DevKit |
| USB cable | For flashing (micro-USB or USB-C depending on board) |
| **Optional:** Relay module | 2-channel 5V relay connected to GPIO26 and GPIO27 |
| **Optional:** Status LED | Onboard LED on GPIO2 (most boards have this) |
| **Optional:** DHT22 | Temperature/humidity sensor on GPIO4 |

Nothing extra is required to get the dashboard working. Relays and sensors are opt-in via `config.h`.

---

## Wiring (optional relay module)

```
ESP32 GPIO26 → Relay 1 IN
ESP32 GPIO27 → Relay 2 IN
ESP32 5V     → Relay VCC
ESP32 GND    → Relay GND
```

---

## Project Structure

```
ESP32-Tomato/
├── src/
│   ├── main.ino            ← Entry point, setup(), loop()
│   ├── config.h            ← All pin defs, constants, feature flags
│   ├── storage.h / .cpp    ← Persistent settings (ESP32 Preferences)
│   ├── wifi_manager.h/.cpp ← WiFi STA + AP fallback + auto-reconnect
│   ├── system_monitor.h/.cpp ← Heap, temp, ping, uptime
│   ├── relay_manager.h/.cpp  ← Dual relay + timer control
│   ├── mqtt_manager.h/.cpp   ← MQTT publish/subscribe
│   ├── ota_manager.h/.cpp    ← ArduinoOTA firmware update
│   ├── led_manager.h/.cpp    ← Non-blocking LED patterns
│   └── web_server.h/.cpp     ← Async HTTP + WebSocket dashboard
├── data/
│   ├── style.css           ← Dashboard stylesheet (dark Tomato theme)
│   └── app.js              ← Frontend JS (WebSocket, relay API, scan)
├── scripts/
│   └── build_spiffs.py     ← PlatformIO pre-build helper
├── platformio.ini          ← PlatformIO project config
└── README.md
```

---

## Setup — PlatformIO (Recommended)

PlatformIO handles all library dependencies automatically.

```bash
# 1. Clone the repo
git clone https://github.com/YOUR_USERNAME/ESP32-Tomato.git
cd ESP32-Tomato

# 2. Open in VS Code with PlatformIO extension installed

# 3. Configure your settings
#    Edit src/config.h — change WiFi AP password, admin password, etc.

# 4. Upload the web files to SPIFFS (do this FIRST)
pio run --target uploadfs

# 5. Build and upload the firmware
pio run --target upload

# 6. Monitor serial output
pio device monitor --baud 115200
```

---

## Setup — Arduino IDE

### Install board support

1. Open Arduino IDE → **File → Preferences**
2. Add to Additional Board Manager URLs:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **Tools → Board → Boards Manager** → search `esp32` → Install **esp32 by Espressif Systems**

### Install libraries

**Sketch → Include Library → Manage Libraries**, install:

| Library | Author |
|---|---|
| ESP Async WebServer | lacamera / ESP32 fork |
| AsyncTCP | dvarrel |
| ArduinoJson | Benoit Blanchon (v6) |
| PubSubClient | Nick O'Leary |
| ESP32Ping | marian-craciunescu |

### Board settings

| Setting | Value |
|---|---|
| Board | ESP32 Dev Module |
| Partition Scheme | Default 4MB with SPIFFS |
| Upload Speed | 921600 |
| Flash Frequency | 80MHz |

### Upload SPIFFS (web files)

1. Install **Arduino ESP32 Filesystem Uploader** plugin
2. Place files from `data/` into a `data/` folder inside your sketch folder
3. **Tools → ESP32 Sketch Data Upload**
4. Then upload the sketch normally

---

## First Boot

On first boot with no saved credentials, the device starts a setup AP:

```
WiFi SSID: ESP32-Tomato
Password:  tomato123
```

Connect to this network, then open **http://192.168.4.1** in your browser.

Log in with:
```
Username: admin
Password: tomato
```

Go to **WiFi** → connect to your home network. The device restarts and connects automatically.

---

## Dashboard

Once connected, open `http://[device-ip]` in your browser (the IP is printed to Serial on boot).

### Pages

| Page | URL | Description |
|---|---|---|
| Dashboard | `/` | Live stats: WiFi, system, relays, network |
| Relays | `/relays` | Toggle relays, set timed actions |
| WiFi | `/wifi` | Scan and connect to networks |
| Config | `/config` | Device name, timezone, MQTT settings |
| Logs | `/logs` | System event log, auto-refreshes |

### REST API

All endpoints require HTTP Basic Auth (admin / your password).

| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/status` | Full system status JSON |
| GET | `/api/stats` | Lightweight stats JSON |
| GET | `/api/scan` | WiFi network scan |
| GET | `/api/logs` | System log entries |
| GET | `/api/ping` | Health check |
| POST | `/api/relay/1` | `state=on\|off` — control relay 1 |
| POST | `/api/relay/2` | `state=on\|off` — control relay 2 |
| POST | `/api/relay/1/timer` | `state=on\|off&delay=5000` — timed relay |
| POST | `/api/relay/2/timer` | Same for relay 2 |
| POST | `/api/restart` | Restart the device |
| POST | `/api/factory-reset` | Wipe all settings and restart |

---

## MQTT

MQTT is optional. Configure a broker in **Config → MQTT**.

### Topics (published every 30 seconds)

```
esp32tomato/{device-name}/heap/free      → free heap bytes
esp32tomato/{device-name}/heap/pct       → heap used percentage
esp32tomato/{device-name}/temp           → CPU temperature °C
esp32tomato/{device-name}/uptime         → uptime in seconds
esp32tomato/{device-name}/rssi           → WiFi signal dBm
esp32tomato/{device-name}/ping           → ping latency ms
esp32tomato/{device-name}/internet       → 1 = online, 0 = offline
esp32tomato/{device-name}/relay/1        → relay 1 state (retained)
esp32tomato/{device-name}/relay/2        → relay 2 state (retained)
esp32tomato/{device-name}/status         → "online" (retained)
```

### Topics (subscribed — send commands)

```
esp32tomato/{device-name}/relay/1/set    → "1" or "on" to turn on, "0" or "off" to turn off
esp32tomato/{device-name}/relay/2/set    → same for relay 2
esp32tomato/{device-name}/restart        → any payload to restart device
```

### Home Assistant example

```yaml
switch:
  - platform: mqtt
    name: "ESP32 Relay 1"
    command_topic: "esp32tomato/ESP32-Tomato/relay/1/set"
    state_topic:   "esp32tomato/ESP32-Tomato/relay/1"
    payload_on:  "1"
    payload_off: "0"
    retain: true

sensor:
  - platform: mqtt
    name: "ESP32 Temperature"
    state_topic: "esp32tomato/ESP32-Tomato/temp"
    unit_of_measurement: "°C"
```

---

## OTA Updates

OTA is enabled by default when WiFi is connected.

### PlatformIO

```bash
pio run --target upload --upload-port 192.168.1.xxx
```

### Arduino IDE

**Sketch → Upload** — select the device from the port list (it appears as a network port).

OTA password is your admin password (default: `tomato`). Change it in Config.

---

## Factory Reset

**Via button:** Hold the BOOT button (GPIO0) for 3 seconds. The LED blinks fast to confirm, then the device resets.

**Via web:** Dashboard → Factory Reset button (requires confirmation).

**Via MQTT:** Publish any payload to `esp32tomato/{device}/restart`.

---

## Configuration

Everything is in `src/config.h`. Key settings:

```cpp
// Change the setup AP password (min 8 chars)
#define WIFI_AP_PASSWORD     "tomato123"

// Change the web dashboard password
#define WEB_AUTH_PASS        "tomato"

// Enable relays if you have them wired up
#define FEATURE_RELAY        false   // → true

// Enable DHT22 if you have one on GPIO4
#define FEATURE_DHT          false   // → true

// Set your timezone (POSIX format)
#define NTP_TIMEZONE         "UTC0"
// Examples:
// "GMT0BST,M3.5.0/1,M10.5.0"          London
// "CET-1CEST,M3.5.0,M10.5.0/3"        Berlin
// "IST-5:30"                           India
// "EST5EDT,M3.2.0,M11.1.0"            New York

// Ping interval
#define INTERVAL_PING_MS     10000   // 10 seconds

// MQTT publish interval
#define INTERVAL_UPLOAD_MS   30000   // 30 seconds
```

---

## LED Status Indicator

| Pattern | Meaning |
|---|---|
| Fast blink (5Hz) | Booting / error / OTA update |
| Slow blink (1Hz) | Connecting to WiFi / AP mode |
| Heartbeat (2 quick + pause) | Connected and running normally |
| Solid ON | OTA complete (before reboot) |

---

## Troubleshooting

**Can't connect to dashboard**
- Check Serial Monitor (115200 baud) for the device IP
- Make sure you're on the same network
- Try `http://esp32tomato.local` (mDNS)

**Web UI shows broken layout / no styles**
- SPIFFS files weren't uploaded
- Run `pio run --target uploadfs` first
- In Arduino IDE: Tools → ESP32 Sketch Data Upload

**MQTT not connecting**
- Verify broker IP and port in Config → MQTT
- Check broker allows anonymous connections or enter credentials
- Check Serial Monitor for `[MQTT]` log lines

**Relay not responding**
- Set `FEATURE_RELAY true` in `config.h`
- Check GPIO26/27 are correctly wired
- Relay modules often need 5V VCC even if control signal is 3.3V

**OTA not appearing as a port**
- Make sure device is on same network as your computer
- Restart Arduino IDE after device connects to WiFi
- Firewall may be blocking UDP port 5353 (mDNS)

---

## License

MIT — use it, build on it, make something useful.
