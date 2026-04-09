#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

// ===== WIFI =====
const char* ssid = "G34";
const char* password = "11223344";

// ===== TELEGRAM =====
#define BOTtoken "8383148693:AAHshbjnLSSbZN75Pal0qDqwJDc9Tr6QYRc"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// ===== OWNER =====
String owner = "5609607409";

// ===== LED PINS =====
int leds[7] = {D1, D2, D5, D6, D7, D0, D8};

// ===== SYSTEM =====
String otp = "";
bool unlocked = false;
bool otpActive = false;

// ⏳ SESSION TIMER
unsigned long unlockTime = 0;
unsigned long sessionTime = 120000;

unsigned long lastTime = 0;
unsigned long delayTime = 150;

// ================= STATUS =================
String getStatus() {

  String s = "🏠 SMART ROCKET LAUNCHER\n\n";

  for (int i = 0; i < 7; i++) {
    s += "ROCKET" + String(i + 1) + ": ";
    s += digitalRead(leds[i]) ? "🟢 ON\n" : "🔴 OFF\n";
  }

  return s;
}

// ================= KEYBOARD =================
String buildKeyboard() {

  String json = "[";

  for (int i = 0; i < 7; i++) {

    String state = digitalRead(leds[i]) ? "OFF" : "ON";

    json += "[{\"text\":\"ROCKET" + String(i + 1) + " " + state +
            "\",\"callback_data\":\"L" + String(i + 1) + "\"}]";

    if (i < 6) json += ",";
  }

  json += "]";

  return json;
}

// ================= SEND PANEL =================
void sendPanel(String chat_id) {

  bot.sendMessageWithInlineKeyboard(
    chat_id,
    getStatus(),
    "",
    buildKeyboard()
  );
}

// ================= SEND OTP =================
void sendOTP() {

  if (otpActive) return;

  otpActive = true;
  unlocked = false;

  otp = "";

  for (int i = 0; i < 4; i++) {
    otp += String(random(0, 10));
  }

  bot.sendMessage(owner, "🔐 Your OTP: " + otp, "");
  bot.sendMessage(owner, "👉 Send OTP to unlock", "");
}

// ================= VERIFY OTP =================
void verifyOTP(String input) {

  if (input == otp) {

    unlocked = true;
    otpActive = false;

    unlockTime = millis();

    bot.sendMessage(owner,
      "✅ ACCESS GRANTED\n⏳ Session started (2 min)", "");

    sendPanel(owner);
  }
  else {
    bot.sendMessage(owner, "❌ WRONG OTP", "");
  }
}

// ================= HANDLE LED =================
void handleCommand(String data, String chat_id) {

  if (chat_id != owner) {
    bot.sendMessage(chat_id, "⛔ Owner only", "");
    return;
  }

  if (!unlocked) {
    bot.sendMessage(chat_id, "🔐 Send /otp first", "");
    return;
  }

  if (data.startsWith("L")) {

    int n = data.substring(1).toInt() - 1;

    digitalWrite(leds[n],
      !digitalRead(leds[n]));

    sendPanel(chat_id);
  }
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  for (int i = 0; i < 7; i++) {
    pinMode(leds[i], OUTPUT);
    digitalWrite(leds[i], LOW);
  }

  WiFi.begin(ssid, password);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }

  client.setInsecure();
  randomSeed(analogRead(A0));

  bot.sendMessage(owner,
    "🤖 BOT ONLINE\nSend /otp", "");
}

// ================= LOOP =================
void loop() {

  // AUTO LOCK
  if (unlocked && millis() - unlockTime > sessionTime) {

    unlocked = false;

    bot.sendMessage(owner,
      "🔒 Session expired\nSend /otp again", "");
  }

  if (millis() - lastTime > delayTime) {

    int numNewMessages =
      bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {

      for (int i = 0; i < numNewMessages; i++) {

        String chat_id = bot.messages[i].chat_id;
        String text = bot.messages[i].text;

        // START
        if (text == "/start") {
          bot.sendMessage(chat_id,
            "Send /otp to unlock", "");
        }

        // OTP REQUEST
        else if (text == "/otp") {
          sendOTP();
        }

        // OTP INPUT
        else if (otpActive && text.length() == 4) {
          verifyOTP(text);
        }

        // BUTTON PRESS
        else if (bot.messages[i].type == "callback_query") {

          String data = bot.messages[i].callback_query_data;
          handleCommand(data, chat_id);
        }
      }

      numNewMessages =
        bot.getUpdates(bot.last_message_received + 1);
    }

    lastTime = millis();
  }
}
