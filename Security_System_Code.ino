#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// --- Configuration ---
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const String BOT_TOKEN = "YOUR_BOT_TOKEN";
const String AUTHORIZED_CHAT_ID = "YOUR_CHAT_ID";

// --- Hardware Pins ---
const int irPin     = 27;
const int buzzerPin = 26;

// --- Shared State (Multicore) ---
volatile bool systemArmed      = true;
volatile bool alarmActive      = false;
volatile bool armingPending    = false;
volatile unsigned long scheduledArmingTime = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

WiFiClientSecure secureClient;
UniversalTelegramBot bot(BOT_TOKEN, secureClient);

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text    = bot.messages[i].text;

    if (chat_id != AUTHORIZED_CHAT_ID) {
      bot.sendMessage(chat_id, "⛔ Access denied.", "");
      continue;
    }

    if (text == "Status") {
      String status = "System Status:\n";
      status += systemArmed ? "🛡️ ARMED\n" : "💤 SLEEP MODE\n";
      status += alarmActive ? "🚨 ALARM ACTIVE!" : "✅ SECURE";
      bot.sendMessage(chat_id, status, "");
    } 
    else if (text == "Turn off alarm") {
      portENTER_CRITICAL(&mux);
      alarmActive = false;
      systemArmed = false;
      portEXIT_CRITICAL(&mux);
      digitalWrite(buzzerPin, LOW);
      bot.sendMessage(chat_id, "🔇 Alarm deactivated. System in sleep mode.", "");
    } 
    else if (text == "Activate alarm") {
      bot.sendMessage(chat_id, "🛡️ Activation in 15s... Please leave the room.", "");
      scheduledArmingTime = millis() + 15000;
      armingPending = true;
    }
  }
}

// --- Core 0: Real-time Detection & Buzzer ---
void detectionTask(void* param) {
  unsigned long lastBeep = 0;
  bool buzzerState = false;
  bool lastIRState = HIGH;
  unsigned long debounceTime = 0;
  const int DEBOUNCE_MS = 50;

  while (true) {
    bool irReading = digitalRead(irPin);
    unsigned long now = millis();

    if (irReading != lastIRState) debounceTime = now;

    if ((now - debounceTime) > DEBOUNCE_MS) {
      if (irReading == LOW) {
        portENTER_CRITICAL(&mux);
        if (systemArmed && !alarmActive) {
          alarmActive = true;
          Serial.println(">>> INTRUSION DETECTED <<<");
        }
        portEXIT_CRITICAL(&mux);
      }
    }
    lastIRState = irReading;

    if (armingPending && now >= scheduledArmingTime) {
      portENTER_CRITICAL(&mux);
      systemArmed = true;
      alarmActive = false;
      armingPending = false;
      portEXIT_CRITICAL(&mux);
      Serial.println("System ARMED.");
    }

    if (alarmActive) {
      if (now - lastBeep > 300) {
        buzzerState = !buzzerState;
        digitalWrite(buzzerPin, buzzerState ? HIGH : LOW);
        lastBeep = now;
      }
    } else {
      digitalWrite(buzzerPin, LOW);
    }

    vTaskDelay(1);
  }
}

// --- Core 1: Connectivity & Notifications ---
void telegramTask(void* param) {
  unsigned long lastCheck = 0;
  bool alertSent = false;

  while (true) {
    unsigned long now = millis();

    if (alarmActive && !alertSent) {
      bot.sendMessage(AUTHORIZED_CHAT_ID, "🚨 Alert: Intrusion detected! 🚨", "");
      alertSent = true;
    }
    if (!alarmActive) alertSent = false;

    if (now - lastCheck > 2000) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      lastCheck = now;
    }
    vTaskDelay(10);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(irPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  secureClient.setInsecure();
  bot.sendMessage(AUTHORIZED_CHAT_ID, "🔌 System Online.", "");

  xTaskCreatePinnedToCore(detectionTask, "Detection", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(telegramTask, "Telegram", 8192, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelay(1000);
}