# Security-system-with-an-ESP32

##  Key Features

* **Asynchronous Dual-Core Execution:** Hardware interrupts and sensor polling run on Core 0, while Telegram API requests run on Core 1.
* **Real-time Intrusion Alerts:** Immediate buzzer activation and Serial logging upon detection.
* **Telegram Remote Control:** Arm, disarm, and check system status from anywhere in the world.
* **Secure Access:** Hardcoded `AUTHORIZED_CHAT_ID` filtering ensures only you can control the system.
* **Smart Arming Delay:** A 15-second "Exit Delay" allows you to leave the room before the PIR sensor becomes active.

---

## Technical Architecture

This project utilizes **Task Pinning** to ensure that network latency never compromises physical security.

### Task Distribution

| Task | Core | Priority | Description |
| --- | --- | --- | --- |
| **DetectionTask** | `0` | 2 | Handles IR sensor debouncing, buzzer PWM patterns, and arming logic. |
| **TelegramTask** | `1` | 1 | Manages WiFi connection and polls the Telegram Bot API for new commands. |

### Logic Flow

1. **Core 0** monitors `irPin` (GPIO 27). If triggered while `systemArmed` is true, it sets `alarmActive = true`.
2. **Core 1** monitors the `alarmActive` flag. As soon as it flips, it pushes a notification to the user via Telegram.
3. **Critical Sections:** Uses `portMUX_TYPE` and `portENTER_CRITICAL` to safely share boolean states between cores without race conditions.

---

## 🛠️ Hardware Requirements

* **Controller:** ESP32 DevKit V1
* **Sensor:** IR Motion Sensor (PIR) or Infrared Obstacle Sensor
* **Output:** Active/Passive Buzzer
* **Pinout:**
* **IR Sensor:** `GPIO 27`
* **Buzzer:** `GPIO 26`



---

##  Installation

### 1. Library Dependencies

Ensure you have the following libraries installed in your Arduino IDE:

* `UniversalTelegramBot` by Brian Lough
* `ArduinoJson` (v6.x or later)

### 2. Bot Configuration

1. Message [@BotFather]() on Telegram to create a new bot and get your **API Token**.
2. Message [@IDBot]() to retrieve your unique **Chat ID**.

### 3. Firmware Setup

Update the configuration section in the `.ino` file:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const String BOT_TOKEN = "123456789:ABCDEF...";
const String AUTHORIZED_CHAT_ID = "987654321";

```

---

##  User Guide

Interact with your security system using these Telegram commands:

* **`Status`**: Returns the current state (Armed/Sleep) and if an alarm is currently ringing.
* **`Activate alarm`**: Starts a 15-second countdown. The system arms once the timer expires.
* **`Turn off alarm`**: Immediately silences the buzzer and puts the system into **Sleep Mode**.

---

##  Safety & Security

* **SSL/TLS:** Uses `WiFiClientSecure` with `setInsecure()` for lightweight Telegram communication. (For production, consider verifying the root certificate).
* **Hardware Debouncing:** Software-level debouncing (50ms) is implemented on the IR pin to prevent false positives from electrical noise.

---

**Developed by [Bignon Codjia]**
