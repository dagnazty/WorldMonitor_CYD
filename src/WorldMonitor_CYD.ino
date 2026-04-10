// WorldMonitor CYD Edition
// ESP32 Cheap Yellow Display (ILI9341 320x240 + XPT2046 touch)
// Standalone — no joystick, SD card optional, no extra hardware needed.
//
// Fetches LIVE data from public APIs:
//   - Yahoo Finance (markets, VIX, yields)
//   - CNN Fear & Greed Index (derived from VIX)
//   - USGS Earthquakes
//   - NASA EONET (volcanoes, wildfires, storms)
// Risk scores, news, and forecasts use mock data.
//
// Navigation: on-screen touch buttons at the bottom.
// Config: stored in ESP32 Preferences (NVS) with optional SD card support.

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <Preferences.h>

#include "config.h"
#include "ui.h"
#include "screens.h"
#include "api.h"
#include "mockdata.h"

// Uncomment to enable touch debug (prints raw coordinates every second)
//#define TOUCH_DEBUG

// ── CYD Hardware ─────────────────────────────────────────────────────────
#define TFT_BL    21    // Backlight pin
#define SD_CS_PIN 5     // SD (optional, not required)

// ── Display ──────────────────────────────────────────────────────────────
TFT_eSPI tft = TFT_eSPI();

// ── Preferences (NVS flash storage, replaces SD) ─────────────────────────
Preferences prefs;

// ── Global State ─────────────────────────────────────────────────────────
int currentScreen = 0;
int scrollOffset = 0;
int maxScrollOffset = 0;

unsigned long lastRefresh = 0;
unsigned long refreshInterval = 120000;  // 2 min default

unsigned long lastScreenAdvance = 0;
unsigned long autoAdvanceInterval = 0;    // 0 = disabled

#ifdef TOUCH_DEBUG
unsigned long lastTouchDebug = 0;
#endif

unsigned long lastTouchTime = 0;
const unsigned long TOUCH_DEBOUNCE = 250;  // ms

bool wifiConnected = false;
bool hasNewAlert = false;
int alertLevel = 0;
bool hideNavBar = false; // when true, drawCurrentScreen will skip navigation bar

// Data cache
MarketData    marketData;
FearGreed     fearGreed;
RiskScores    riskScores;
NewsDigest    newsDigest;
NaturalEvents naturalEvents;
Forecasts     forecasts;
bool dataLoaded = false;

Config config;

// Touch button state
struct TouchButton {
  int x, y, w, h;
  const char* label;
  uint16_t color;
};

// Bottom nav bar buttons
#define NAV_BAR_H  36
#define NAV_BTN_W  60
#define NAV_BTN_H  28
#define NAV_Y      (SCREEN_H - NAV_BAR_H + 4)

TouchButton navButtons[5] = {
  {  6,  NAV_Y, NAV_BTN_W, NAV_BTN_H, "<",    COLOR_ACCENT_DIM },  // Left
  { 70,  NAV_Y, NAV_BTN_W, NAV_BTN_H, "UP",    COLOR_BLUE       },  // Up
  {130,  NAV_Y, NAV_BTN_W+28, NAV_BTN_H, "REFR", COLOR_GREEN_DIM },  // Center/Refresh
  {222,  NAV_Y, NAV_BTN_W, NAV_BTN_H, "DOWN",  COLOR_BLUE       },  // Down
  {256,  NAV_Y, NAV_BTN_W, NAV_BTN_H, ">",     COLOR_ACCENT_DIM },  // Right
};

// ── Setup ────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println(F("\n=== WorldMonitor CYD ==="));

  // Backlight on
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Load config (SD first, then NVS) — before display to avoid SPI conflict
  loadConfig(config);
  refreshInterval = config.refreshSeconds * 1000UL;
  if (refreshInterval < 60000) refreshInterval = 60000;
  autoAdvanceInterval = config.autoAdvanceSeconds * 1000UL;
  lastScreenAdvance = millis();

  // Display init (TFT_eSPI — handles ILI9341 + XPT2046 touch)
  tft.init();
  tft.setRotation(1);        // Landscape 320x240
  tft.invertDisplay(true);   // CYD needs inverted colors for proper dark mode
  tft.fillScreen(COLOR_BG);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setTextWrap(false);

  // Touch calibration (TFT_eSPI built-in XPT2046)
  // Calibrated values for typical CYD 2.8" — adjust if yours is different
  uint16_t calData[5] = { 280, 3520, 340, 3580, 3 };
  tft.setTouch(calData);

  Serial.println(F("Display + touch ready"));

  // Load mock data immediately
  loadMockRiskNewsForecasts(riskScores, newsDigest, forecasts);
  dataLoaded = true;

  // Splash
  drawSplashScreen(tft);
  delay(1500);

  // Draw dashboard immediately (mock data)
  drawCurrentScreen();
  Serial.println(F("Dashboard drawn, connecting WiFi..."));

  // WiFi
  connectWiFi();

  // Fetch live data if connected
  if (wifiConnected) {
    drawStatusBar(tft, "Fetching live data...", COLOR_ACCENT);
    fetchLiveData();
    lastRefresh = millis();
    drawCurrentScreen();
  }
}

// ── Main Loop ────────────────────────────────────────────────────────────
void loop() {
  handleTouch();

  // Periodic refresh
  if (wifiConnected && (millis() - lastRefresh > refreshInterval)) {
    drawStatusBar(tft, "Refreshing...", COLOR_ACCENT);
    fetchLiveData();
    lastRefresh = millis();
    drawCurrentScreen();
  }

  // Auto‑advance screens (hide nav bar, skip settings)
  if (autoAdvanceInterval > 0 && (millis() - lastScreenAdvance > autoAdvanceInterval)) {
    lastScreenAdvance = millis();
    int screenCount = screenCountForConfig(config);
    // Skip the settings screen (last index) during auto‑advance
    int screensForAuto = screenCount - 1; // exclude settings
    if (screensForAuto <= 0) screensForAuto = 1; // fallback if only settings exists
    currentScreen = (currentScreen + 1) % screensForAuto;
    // If we landed on settings (should not happen with modulo), jump to 0
    if (currentScreen == screenCount - 1) currentScreen = 0;
    scrollOffset = 0;
    hideNavBar = true;
    drawCurrentScreen();
    hideNavBar = false;
  }

#ifdef TOUCH_DEBUG
  // Touch debug: print raw coordinates every second
  if (millis() - lastTouchDebug > 1000) {
    lastTouchDebug = millis();
    uint16_t rawX, rawY;
    bool pressed = tft.getTouch(&rawX, &rawY);
    Serial.printf("Touch debug: pressed=%d rawX=%u rawY=%u\n", pressed, rawX, rawY);
  }
#endif

  // WiFi reconnection
  if (!wifiConnected && WiFi.status() != WL_CONNECTED && (millis() % 30000 < 10)) {
    connectWiFi();
  }

  delay(10);
}

// ── WiFi ──────────────────────────────────────────────────────────────────
void connectWiFi() {
  if (strlen(config.wifiSSID) == 0 || strcmp(config.wifiSSID, "YourWiFi") == 0) {
    Serial.println(F("No WiFi credentials configured"));
    return;
  }

  drawStatusBar(tft, "WiFi...", COLOR_YELLOW);
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSSID, config.wifiPassword);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\nWiFi: %s\n", WiFi.localIP().toString().c_str());
    drawStatusBar(tft, "WiFi OK", COLOR_GREEN);
  } else {
    wifiConnected = false;
    Serial.println(F("\nWiFi failed"));
    drawStatusBar(tft, "WiFi FAIL", COLOR_RED);
  }
}

// ── Touch Handling ───────────────────────────────────────────────────────
void handleTouch() {
  if (millis() - lastTouchTime < TOUCH_DEBOUNCE) return;

  uint16_t rawX, rawY;
  bool pressed = tft.getTouch(&rawX, &rawY);
  if (!pressed) return;
  lastScreenAdvance = millis();  // Reset auto‑advance timer on any touch

  // TFT_eSPI getTouch() already returns calibrated screen coords in rotation
  // But verify: rawX should be 0-319, rawY 0-239 in landscape
  int tx = rawX;
  int ty = rawY;

  // Clamp
  if (tx < 0) tx = 0;
  if (tx >= SCREEN_W) tx = SCREEN_W - 1;
  if (ty < 0) ty = 0;
  if (ty >= SCREEN_H) ty = SCREEN_H - 1;

  Serial.printf("Touch: (%d, %d)\n", tx, ty);

  // Check nav buttons (bottom bar) — only if nav bar is visible
  if (!hideNavBar) {
    for (int i = 0; i < 5; i++) {
      TouchButton& btn = navButtons[i];
      if (tx >= btn.x && tx < btn.x + btn.w && ty >= btn.y && ty < btn.y + btn.h) {
        lastTouchTime = millis();
        flashNavButton(i);
        onNavButton(i);
        return;
      }
    }
  }

  // Tap in content area = center action (refresh / toggle)
  if (ty < (SCREEN_H - NAV_BAR_H)) {
    lastTouchTime = millis();
    int screenCount = screenCountForConfig(config);
    if (currentScreen == screenCount - 1) {
      // Settings screen — toggle UI mode
      toggleUiMode();
    } else {
      // Refresh
      if (wifiConnected) {
        drawStatusBar(tft, "Refreshing...", COLOR_ACCENT);
        fetchLiveData();
        lastRefresh = millis();
      }
    }
    drawCurrentScreen();
  }
}

void onNavButton(int idx) {
  int screenCount = screenCountForConfig(config);

  switch (idx) {
    case 0: // Left
      currentScreen = (currentScreen - 1 + screenCount) % screenCount;
      scrollOffset = 0;
      break;
    case 1: // Up
      if (scrollOffset > 0) scrollOffset--;
      break;
    case 2: // Center — refresh or toggle
      if (currentScreen == screenCount - 1) {
        toggleUiMode();
      } else if (wifiConnected) {
        drawStatusBar(tft, "Refreshing...", COLOR_ACCENT);
        fetchLiveData();
        lastRefresh = millis();
      }
      break;
    case 3: // Down
      if (scrollOffset < maxScrollOffset) scrollOffset++;
      break;
    case 4: // Right
      currentScreen = (currentScreen + 1) % screenCount;
      scrollOffset = 0;
      break;
  }

  drawCurrentScreen();
}

void flashNavButton(int idx) {
  TouchButton& btn = navButtons[idx];
  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 4, COLOR_ACCENT);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_BG, COLOR_ACCENT);
  tft.setCursor(btn.x + (btn.w - strlen(btn.label) * 6) / 2, btn.y + (btn.h - 8) / 2);
  tft.print(btn.label);
  delay(80);
}

void toggleUiMode() {
  bool switchingToClassic = isSimpleUiMode(config);
  strlcpy(config.uiMode, switchingToClassic ? "classic" : "simple", sizeof(config.uiMode));
  currentScreen = screenCountForConfig(config) - 1;
  scrollOffset = 0;
  maxScrollOffset = 0;
  saveConfig(config);
  Serial.printf("UI mode -> %s\n", config.uiMode);
}

// ── Live Data ────────────────────────────────────────────────────────────
void fetchLiveData() {
  Serial.println(F("=== Fetching live data ==="));
  fetchMarketData(marketData);
  delay(500);
  fetchFearGreed(fearGreed);
  delay(500);
  fetchNaturalEvents(naturalEvents);
  checkAlerts();
  Serial.println(F("=== Live data complete ==="));
}

// ── Screen Router ─────────────────────────────────────────────────────────
void drawCurrentScreen() {
  int screenCount = screenCountForConfig(config);
  if (currentScreen >= screenCount) currentScreen = 0;

  // Content area: clear above nav bar (or full screen if nav bar hidden)
  int contentHeight = hideNavBar ? SCREEN_H : SCREEN_H - NAV_BAR_H;
  tft.fillRect(0, 0, SCREEN_W, contentHeight, COLOR_BG);

  if (isSimpleUiMode(config)) {
    switch (currentScreen) {
      case 0: drawSimpleSummaryScreen(tft, marketData, fearGreed, riskScores, wifiConnected, alertLevel); break;
      case 1: drawSimpleMarketsScreen(tft, marketData, wifiConnected, scrollOffset, maxScrollOffset); break;
      case 2: drawSimpleAlertsScreen(tft, riskScores, newsDigest, naturalEvents, alertLevel); break;
      case 3: drawSimpleSettingsScreen(tft, config, wifiConnected); break;
    }
  } else {
    switch (currentScreen) {
      case 0: drawDashboardScreen(tft, marketData, fearGreed, riskScores); break;
      case 1: drawMarketsScreen(tft, marketData, fearGreed, scrollOffset, maxScrollOffset); break;
      case 2: drawRiskScreen(tft, riskScores, scrollOffset, maxScrollOffset); break;
      case 3: drawNewsScreen(tft, newsDigest, scrollOffset, maxScrollOffset); break;
      case 4: drawNaturalScreen(tft, naturalEvents, scrollOffset, maxScrollOffset); break;
      case 5: drawForecastScreen(tft, forecasts, scrollOffset, maxScrollOffset); break;
      case 6: drawSettingsScreen(tft, config, wifiConnected, scrollOffset); break;
    }
  }

  // Draw navigation bar unless hidden
  if (!hideNavBar) {
    drawNavBar();
  }
}

// ── Navigation Bar ───────────────────────────────────────────────────────
void drawNavBar() {
  // Bar background
  tft.fillRect(0, SCREEN_H - NAV_BAR_H, SCREEN_W, NAV_BAR_H, COLOR_PANEL);
  tft.drawFastHLine(0, SCREEN_H - NAV_BAR_H, SCREEN_W, COLOR_BORDER);

  // Draw buttons
  for (int i = 0; i < 5; i++) {
    TouchButton& btn = navButtons[i];
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 4, btn.color);
    tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 4, COLOR_BORDER);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, btn.color);
    int textW = strlen(btn.label) * 6;
    tft.setCursor(btn.x + (btn.w - textW) / 2, btn.y + (btn.h - 8) / 2);
    tft.print(btn.label);
  }

  // Screen indicator dots
  int screenCount = screenCountForConfig(config);
  int dotY = SCREEN_H - 5;
  int gap = 10;
  int totalW = (screenCount - 1) * gap + 6;
  int startX = (SCREEN_W - totalW) / 2;
  for (int i = 0; i < screenCount; i++) {
    if (i == currentScreen) {
      tft.fillCircle(startX + i * gap, dotY, 2, COLOR_ACCENT);
    } else {
      tft.fillCircle(startX + i * gap, dotY, 1, COLOR_TEXT_MUTED);
    }
  }
}

// ── Alerts ───────────────────────────────────────────────────────────────
void checkAlerts() {
  int prevLevel = alertLevel;
  alertLevel = 0;

  for (int i = 0; i < riskScores.count; i++) {
    if (riskScores.items[i].combinedScore >= 80) alertLevel = max(alertLevel, 4);
    else if (riskScores.items[i].combinedScore >= 65) alertLevel = max(alertLevel, 3);
  }

  for (int i = 0; i < newsDigest.count; i++) {
    if (newsDigest.items[i].threatLevel >= 3) alertLevel = max(alertLevel, 3);
  }

  hasNewAlert = (alertLevel > prevLevel);
}

// ── NVS Config ───────────────────────────────────────────────────────────
void loadConfigNVS(Config& cfg) {
  setDefaultConfig(cfg);
  prefs.begin("wmconfig", true);  // read-only
  if (prefs.isKey("ssid")) {
    strlcpy(cfg.wifiSSID,     prefs.getString("ssid", "").c_str(),     sizeof(cfg.wifiSSID));
    strlcpy(cfg.wifiPassword, prefs.getString("pass", "").c_str(),     sizeof(cfg.wifiPassword));
    strlcpy(cfg.uiMode,       prefs.getString("ui", "simple").c_str(),  sizeof(cfg.uiMode));
    cfg.refreshSeconds = prefs.getInt("refresh", 60);
    cfg.buzzerEnabled  = prefs.getBool("buzzer", false);
    cfg.brightness     = prefs.getUChar("bright", 200);
    cfg.autoAdvanceSeconds = prefs.getInt("advance", 30);
    Serial.println(F("Config loaded from NVS"));
  } else {
    Serial.println(F("No NVS config, using defaults"));
  }
  prefs.end();
}

void saveConfigNVS(const Config& cfg) {
  prefs.begin("wmconfig", false);  // read-write
  prefs.putString("ssid",    cfg.wifiSSID);
  prefs.putString("pass",    cfg.wifiPassword);
  prefs.putString("ui",      cfg.uiMode);
  prefs.putInt("refresh",    cfg.refreshSeconds);
  prefs.putBool("buzzer",   cfg.buzzerEnabled);
  prefs.putUChar("bright",   cfg.brightness);
  prefs.putInt("advance",   cfg.autoAdvanceSeconds);
  prefs.end();
  Serial.println(F("Config saved to NVS"));
}

// ── SD Card Config (optional) ───────────────────────────────────────────
bool mountSD() {
  SPI.begin();
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  if (SD.begin(SD_CS_PIN, SPI)) {
    Serial.println(F("SD card mounted"));
    return true;
  }
  Serial.println(F("SD card not found"));
  SPI.end();
  return false;
}

void unmountSD() {
  SD.end();
  SPI.end();
  Serial.println(F("SD unmounted"));
}

bool loadConfigSD(Config& cfg) {
  if (!mountSD()) return false;
  
  File file = SD.open("/wmconfig.json", FILE_READ);
  if (!file) {
    Serial.println(F("No config file on SD"));
    unmountSD();
    return false;
  }
  
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  unmountSD();
  
  if (err) {
    Serial.printf("SD config parse error: %s\n", err.c_str());
    return false;
  }
  
  if (doc["wifi_ssid"])      strlcpy(cfg.wifiSSID,     doc["wifi_ssid"],     sizeof(cfg.wifiSSID));
  if (doc["wifi_password"])  strlcpy(cfg.wifiPassword, doc["wifi_password"], sizeof(cfg.wifiPassword));
  if (doc["api_key"])        strlcpy(cfg.apiKey,       doc["api_key"],       sizeof(cfg.apiKey));
  if (doc["api_base_url"])   strlcpy(cfg.apiBaseURL,   doc["api_base_url"],  sizeof(cfg.apiBaseURL));
  if (doc["ui_mode"])        strlcpy(cfg.uiMode,       doc["ui_mode"],       sizeof(cfg.uiMode));
  if (doc.containsKey("buzzer"))          cfg.buzzerEnabled  = doc["buzzer"];
  if (doc.containsKey("brightness"))      cfg.brightness     = doc["brightness"];
  if (doc.containsKey("refresh_seconds")) cfg.refreshSeconds = doc["refresh_seconds"];
  if (doc.containsKey("screen_timeout"))  cfg.screenTimeout  = doc["screen_timeout"];
  if (doc.containsKey("auto_advance_seconds")) cfg.autoAdvanceSeconds = doc["auto_advance_seconds"];
  
  Serial.println(F("Config loaded from SD"));
  return true;
}

bool saveConfigSD(const Config& cfg) {
  if (!mountSD()) return false;
  
  JsonDocument doc;
  doc["wifi_ssid"]       = cfg.wifiSSID;
  doc["wifi_password"]   = cfg.wifiPassword;
  doc["api_key"]         = cfg.apiKey;
  doc["api_base_url"]    = cfg.apiBaseURL;
  doc["ui_mode"]         = cfg.uiMode;
  doc["buzzer"]          = cfg.buzzerEnabled;
  doc["brightness"]      = cfg.brightness;
  doc["refresh_seconds"] = cfg.refreshSeconds;
  doc["screen_timeout"]  = cfg.screenTimeout;
  doc["auto_advance_seconds"] = cfg.autoAdvanceSeconds;
  
  if (SD.exists("/wmconfig.json")) {
    SD.remove("/wmconfig.json");
  }
  
  File file = SD.open("/wmconfig.json", FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to open SD for writing"));
    unmountSD();
    return false;
  }
  
  serializeJsonPretty(doc, file);
  file.close();
  unmountSD();
  Serial.println(F("Config saved to SD"));
  return true;
}

// ── Unified Config ──────────────────────────────────────────────────────
void loadConfig(Config& cfg) {
  // Try SD first
  if (loadConfigSD(cfg)) {
    // Save to NVS for persistence (optional)
    saveConfigNVS(cfg);
    return;
  }
  // Fall back to NVS
  loadConfigNVS(cfg);
}

void saveConfig(const Config& cfg) {
  // Save to both SD (if present) and NVS
  saveConfigSD(cfg);  // will fail silently if no SD
  saveConfigNVS(cfg);
}
