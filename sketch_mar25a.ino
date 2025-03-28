#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include "config.h" // Archivo local con credenciales

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

bool monitoring = true;
bool epicNotifications = true;
float maxSessionDiff1 = 0, maxSessionDiff2 = 0;
float lastEpicDiff1 = 0, lastEpicDiff2 = 0;
const float EPIC_DIFF_THRESHOLD = 100000000;
const float TEMP_THRESHOLD = 65.0;
const float HASH_THRESHOLD = 100.0;
unsigned long lastReportTime = 0;
const unsigned long REPORT_INTERVAL = 6 * 60 * 60 * 1000;
unsigned long lowHashTime1 = 0, lowHashTime2 = 0;
unsigned long lastTempAlert1 = 0, lastTempAlert2 = 0;
const unsigned long TEMP_ALERT_INTERVAL = 10 * 60 * 1000;
const unsigned long LOW_HASH_TIMEOUT = 5 * 60 * 1000;
unsigned long lastResetTime1 = 0, lastResetTime2 = 0;
const unsigned long RESET_COOLDOWN = 30 * 60 * 1000;
String epicHistory[5];
int epicHistoryIndex = 0;
float totalHashrate1 = 0, totalHashrate2 = 0;
int hashCount1 = 0, hashCount2 = 0;
float maxHistoricalDiff1 = 0, maxHistoricalDiff2 = 0;
unsigned long statsResetTime = 0;
const unsigned long STATS_INTERVAL = 24 * 60 * 60 * 1000;
float dailyHashrate1[7] = {0}; // Estadísticas de 7 días
float dailyHashrate2[7] = {0};
int dayIndex = 0;
unsigned long lastDayReset = 0;
const unsigned long DAY_INTERVAL = 24 * 60 * 60 * 1000;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 Booting...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to Wi-Fi...");
  }
  Serial.println("Connected to Wi-Fi! IP: " + WiFi.localIP().toString());

  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  bot.sendMessage(CHAT_ID, "ESP32 started - Monitoring Mi Bitaxe Supra & Ultra");
  statsResetTime = millis();
  lastDayReset = millis();
}

void loop() {
  if (monitoring) {
    checkSessionBest(bitaxeIP1, "Mi Bitaxe Supra", 1);
    checkSessionBest(bitaxeIP2, "Mi Bitaxe Ultra", 2);
  }

  if (millis() - lastReportTime >= REPORT_INTERVAL) {
    sendReport();
    lastReportTime = millis();
  }

  if (millis() - statsResetTime >= STATS_INTERVAL) {
    resetStats();
    statsResetTime = millis();
  }

  if (millis() - lastDayReset >= DAY_INTERVAL) {
    dailyHashrate1[dayIndex] = (hashCount1 > 0) ? totalHashrate1 / hashCount1 : 0;
    dailyHashrate2[dayIndex] = (hashCount2 > 0) ? totalHashrate2 / hashCount2 : 0;
    dayIndex = (dayIndex + 1) % 7;
    totalHashrate1 = 0; totalHashrate2 = 0;
    hashCount1 = 0; hashCount2 = 0;
    lastDayReset = millis();
  }

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if (numNewMessages > 0) {
    for (int i = 0; i < numNewMessages; i++) {
      String chat_id = String(bot.messages[i].chat_id);
      String text = bot.messages[i].text;
      if (chat_id != CHAT_ID) continue;

      String command = text;
      if (command.indexOf("@BitaxeMonitorBot") != -1) {
        command = command.substring(0, command.indexOf("@BitaxeMonitorBot"));
      }

      if (command == "/start") {
        monitoring = true;
        bot.sendMessage(CHAT_ID, "Monitoring activated");
      }
      if (command == "/stop") {
        monitoring = false;
        bot.sendMessage(CHAT_ID, "Monitoring paused");
      }
      if (command == "/reset1") resetBitaxe(bitaxeIP1, "Mi Bitaxe Supra", 1);
      if (command == "/reset2") resetBitaxe(bitaxeIP2, "Mi Bitaxe Ultra", 2);
      if (command == "/sessionbest1") sendSessionBest(bitaxeIP1, "Mi Bitaxe Supra");
      if (command == "/sessionbest2") sendSessionBest(bitaxeIP2, "Mi Bitaxe Ultra");
      if (command == "/status1") sendStatus(bitaxeIP1, "Mi Bitaxe Supra");
      if (command == "/status2") sendStatus(bitaxeIP2, "Mi Bitaxe Ultra");
      if (command == "/report") sendReport();
      if (command == "/stats") sendStats();
      if (command == "/epic_on") {
        epicNotifications = true;
        bot.sendMessage(CHAT_ID, "Epic share notifications enabled");
      }
      if (command == "/epic_off") {
        epicNotifications = false;
        bot.sendMessage(CHAT_ID, "Epic share notifications disabled");
      }
      if (command == "/epic_history") sendEpicHistory();
    }
  }

  delay(1000);
}

void checkSessionBest(const char* ip, const char* name, int bitaxeNum) {
  HTTPClient http;
  String url = "http://" + String(ip) + "/api/system/info";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String sessionBestStr = doc["bestSessionDiff"].as<String>();
    float sessionBest = parseDiff(sessionBestStr);
    float hashrate = doc["hashRate"].as<float>();
    float temp = doc["temp"].as<float>();
    float& maxSessionDiff = (bitaxeNum == 1) ? maxSessionDiff1 : maxSessionDiff2;
    float& lastEpicDiff = (bitaxeNum == 1) ? lastEpicDiff1 : lastEpicDiff2;
    float& totalHashrate = (bitaxeNum == 1) ? totalHashrate1 : totalHashrate2;
    int& hashCount = (bitaxeNum == 1) ? hashCount1 : hashCount2;
    float& maxHistoricalDiff = (bitaxeNum == 1) ? maxHistoricalDiff1 : maxHistoricalDiff2;
    unsigned long& lastTempAlert = (bitaxeNum == 1) ? lastTempAlert1 : lastTempAlert2;
    unsigned long& lowHashTime = (bitaxeNum == 1) ? lowHashTime1 : lowHashTime2;
    unsigned long& lastResetTime = (bitaxeNum == 1) ? lastResetTime1 : lastResetTime2;

    totalHashrate += hashrate;
    hashCount++;
    if (sessionBest > maxHistoricalDiff) maxHistoricalDiff = sessionBest;

    if (sessionBest > maxSessionDiff) {
      String message = "New session record on " + String(name) + "! BestDiff: " + sessionBestStr;
      bot.sendMessage(CHAT_ID, message);
      maxSessionDiff = sessionBest;
    }

    if (epicNotifications && sessionBest >= EPIC_DIFF_THRESHOLD && sessionBest > lastEpicDiff) {
      String message = "EPIC SHARE 🤯⚡️ on " + String(name) + "! BestDiff: " + sessionBestStr + " 🚨⛏️ Mining beast unleashed! #Bitaxe #BTC #Mining #Bitcoin #Crypto";
      bot.sendMessage(CHAT_ID, message);
      lastEpicDiff = sessionBest;
      addToEpicHistory(message);
    }

    if (temp >= TEMP_THRESHOLD && (millis() - lastTempAlert >= TEMP_ALERT_INTERVAL || lastTempAlert == 0)) {
      String message = "🚨 ¡Alerta! " + String(name) + " - Temp: " + String(temp) + "°C - ¡Revisá la ventilación! 🚨";
      bot.sendMessage(CHAT_ID, message);
      lastTempAlert = millis();
    }

    if (hashrate < HASH_THRESHOLD) {
      if (lowHashTime == 0) lowHashTime = millis();
      else if (millis() - lowHashTime >= LOW_HASH_TIMEOUT && millis() - lastResetTime >= RESET_COOLDOWN) {
        resetBitaxe(ip, name, bitaxeNum);
        String message = "⚠️ " + String(name) + " reiniciado - Hashrate bajo (" + String(hashrate) + " GH/s)";
        bot.sendMessage(CHAT_ID, message);
        lowHashTime = 0;
        lastResetTime = millis();
      }
    } else {
      lowHashTime = 0;
    }
  }
  http.end();
}

float parseDiff(String diffStr) {
  diffStr.replace(" ", "");
  if (diffStr.indexOf("G") != -1) {
    diffStr.replace("G", "");
    return diffStr.toFloat() * 1000000000;
  } else if (diffStr.indexOf("M") != -1) {
    diffStr.replace("M", "");
    return diffStr.toFloat() * 1000000;
  } else if (diffStr.indexOf("k") != -1) {
    diffStr.replace("k", "");
    return diffStr.toFloat() * 1000;
  }
  return diffStr.toFloat();
}

void resetBitaxe(const char* ip, const char* name, int bitaxeNum) {
  HTTPClient http;
  String url = "http://" + String(ip) + "/api/system/restart";
  http.begin(url);
  int httpCode = http.POST("");
  if (httpCode == HTTP_CODE_OK) {
    bot.sendMessage(CHAT_ID, String(name) + " restarted!");
    if (bitaxeNum == 1) { maxSessionDiff1 = 0; lastEpicDiff1 = 0; lowHashTime1 = 0; }
    if (bitaxeNum == 2) { maxSessionDiff2 = 0; lastEpicDiff2 = 0; lowHashTime2 = 0; }
  } else {
    bot.sendMessage(CHAT_ID, "Error restarting " + String(name));
  }
  http.end();
}

void sendSessionBest(const char* ip, const char* name) {
  HTTPClient http;
  String url = "http://" + String(ip) + "/api/system/info";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String sessionBestStr = doc["bestSessionDiff"].as<String>();
    String message = String(name) + " - Session BestDiff: " + sessionBestStr;
    bot.sendMessage(CHAT_ID, message);
  } else {
    bot.sendMessage(CHAT_ID, "Error fetching session BestDiff for " + String(name));
  }
  http.end();
}

void sendStatus(const char* ip, const char* name) {
  HTTPClient http;
  String url = "http://" + String(ip) + "/api/system/info";
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String bestDiffStr = doc["bestDiff"].as<String>();
    float hashrate = doc["hashRate"].as<float>();
    float temp = doc["temp"].as<float>();
    String message = String(name) + " - Hashrate: " + String(hashrate) + " GH/s, Temp: " + String(temp) + "°C, BestDiff: " + bestDiffStr;
    bot.sendMessage(CHAT_ID, message);
  } else {
    bot.sendMessage(CHAT_ID, "Error fetching status for " + String(name));
  }
  http.end();
}

void sendReport() {
  String report = "📊 *Reporte Diario* 📊\n==================\n";
  HTTPClient http;

  String url1 = "http://" + String(bitaxeIP1) + "/api/system/info";
  http.begin(url1);
  int httpCode1 = http.GET();
  if (httpCode1 == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String bestDiffStr = doc["bestDiff"].as<String>();
    float hashrate = doc["hashRate"].as<float>();
    float temp = doc["temp"].as<float>();
    report += "⛏️ *Mi Bitaxe Supra*\n";
    report += "🤖 Hashrate: " + String(hashrate) + " GH/s | 🌡️ Temp: " + String(temp) + "°C | ⭐ BestDiff: " + bestDiffStr + "\n";
    report += "---\n";
  } else {
    report += "⛏️ *Mi Bitaxe Supra* - Error fetching data\n---\n";
  }
  http.end();

  String url2 = "http://" + String(bitaxeIP2) + "/api/system/info";
  http.begin(url2);
  int httpCode2 = http.GET();
  if (httpCode2 == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String bestDiffStr = doc["bestDiff"].as<String>();
    float hashrate = doc["hashRate"].as<float>();
    float temp = doc["temp"].as<float>();
    report += "⛏️ *Mi Bitaxe Ultra*\n";
    report += "🤖 Hashrate: " + String(hashrate) + " GH/s | 🌡️ Temp: " + String(temp) + "°C | ⭐ BestDiff: " + bestDiffStr + "\n";
  } else {
    report += "⛏️ *Mi Bitaxe Ultra* - Error fetching data\n";
  }
  http.end();

  report += "==================";
  bot.sendMessage(CHAT_ID, report, "Markdown");
}

void sendStats() {
  String stats = "📈 *Estadísticas 7 días* 📈\n";
  stats += "⛏️ *Mi Bitaxe Supra*\n";
  for (int i = 0; i < 7; i++) {
    int idx = (dayIndex - 1 - i + 7) % 7;
    float hr = dailyHashrate1[idx];
    int bars = (hr / 500) * 10; // Escala: 500 GH/s = 10 barras
    String bar = "[" + String("##########").substring(0, bars) + String("----------").substring(bars) + "]";
    stats += "Día -" + String(i + 1) + ": " + bar + " " + String(hr) + " GH/s\n";
  }
  stats += "⛏️ *Mi Bitaxe Ultra*\n";
  for (int i = 0; i < 7; i++) {
    int idx = (dayIndex - 1 - i + 7) % 7;
    float hr = dailyHashrate2[idx];
    int bars = (hr / 500) * 10;
    String bar = "[" + String("##########").substring(0, bars) + String("----------").substring(bars) + "]";
    stats += "Día -" + String(i + 1) + ": " + bar + " " + String(hr) + " GH/s\n";
  }
  bot.sendMessage(CHAT_ID, stats, "Markdown");
}

void resetStats() {
  totalHashrate1 = 0;
  totalHashrate2 = 0;
  hashCount1 = 0;
  hashCount2 = 0;
  maxHistoricalDiff1 = maxSessionDiff1;
  maxHistoricalDiff2 = maxSessionDiff2;
}

void addToEpicHistory(String message) {
  epicHistory[epicHistoryIndex] = message;
  epicHistoryIndex = (epicHistoryIndex + 1) % 5;
}

void sendEpicHistory() {
  String history = "📜 *Historial de Shares Épicos* 📜\n";
  for (int i = 0; i < 5; i++) {
    int idx = (epicHistoryIndex - 1 - i + 5) % 5;
    if (epicHistory[idx] != "") {
      history += String(i + 1) + ". " + epicHistory[idx] + "\n";
    }
  }
  if (history == "📜 *Historial de Shares Épicos* 📜\n") {
    history += "No hay shares épicos registrados aún.";
  }
  bot.sendMessage(CHAT_ID, history, "Markdown");
}