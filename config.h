#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// ── Data Structures ───────────────────────────────────────────────────────

struct Config {
  char wifiSSID[64];
  char wifiPassword[64];
  char apiKey[128];
  char apiBaseURL[128];
  char uiMode[16];
  bool buzzerEnabled;
  int  brightness;
  int  refreshSeconds;
  int  screenTimeout;
  int  autoAdvanceSeconds;
};

struct Quote {
  char symbol[16];
  char name[40];
  float price;
  float change;
  float sparkline[12];
  int sparklineLen;
};

struct MarketData {
  Quote quotes[20];
  int count;
};

struct FearGreed {
  int compositeScore;
  char compositeLabel[24];
  float vix;
  float yield10y;
  float putCallRatio;
  float pctAbove200d;
  char fedRate[16];
  int cnnFearGreed;
};

struct RiskItem {
  char region[32];
  int combinedScore;
  int dynamicScore;
  char trend[8];
  int newsActivity;
  int militaryActivity;
};

struct RiskScores {
  RiskItem items[16];
  int count;
};

struct NewsItem {
  char title[120];
  char source[32];
  char region[32];
  char category[32];
  int  threatLevel;
  bool isAlert;
  float importanceScore;
  unsigned long publishedAt;
};

struct NewsDigest {
  NewsItem items[30];
  int count;
  char worldBrief[512];
};

struct NaturalEvent {
  char title[100];
  char category[24];
  char categoryTitle[32];
  float lat;
  float lon;
  float magnitude;
  char magnitudeUnit[16];
  char sourceName[32];
  unsigned long date;
  char stormName[32];
  int  windKt;
  int  pressureMb;
  int  stormCategory;
};

struct NaturalEvents {
  NaturalEvent items[20];
  int count;
};

struct Forecast {
  char title[100];
  char domain[24];
  char region[32];
  char scenario[120];
  float probability;
  float priorProbability;
  float confidence;
  char timeHorizon[16];
  char trend[16];
};

struct Forecasts {
  Forecast items[16];
  int count;
};

// ── Config Defaults ──────────────────────────────────────────────────────

inline void setDefaultConfig(Config& cfg) {
  strlcpy(cfg.wifiSSID,     "YourWiFi",                   sizeof(cfg.wifiSSID));
  strlcpy(cfg.wifiPassword, "YourPassword",               sizeof(cfg.wifiPassword));
  strlcpy(cfg.apiKey,       "",                            sizeof(cfg.apiKey));
  strlcpy(cfg.apiBaseURL,   "https://worldmonitor.app",   sizeof(cfg.apiBaseURL));
  strlcpy(cfg.uiMode,       "classic",                      sizeof(cfg.uiMode));
  cfg.buzzerEnabled  = false;  // CYD has no buzzer
  cfg.brightness     = 200;
  cfg.refreshSeconds = 60;
  cfg.screenTimeout  = 0;
  cfg.autoAdvanceSeconds = 30;  // Auto‑switch screens every 30 seconds (0 = off)
}

inline bool isClassicUiMode(const Config& cfg) {
  return strcmp(cfg.uiMode, "classic") == 0;
}

inline bool isSimpleUiMode(const Config& cfg) {
  return !isClassicUiMode(cfg);
}

inline int screenCountForConfig(const Config& cfg) {
  return isSimpleUiMode(cfg) ? 4 : 7;
}

// NVS config functions (defined in .ino)
void loadConfigNVS(Config& cfg);
void saveConfigNVS(const Config& cfg);

// SD card config functions (optional, defined in .ino if SD_CS_PIN defined)
bool loadConfigSD(Config& cfg);
bool saveConfigSD(const Config& cfg);

// Unified loader: tries SD first, then NVS, then defaults
void loadConfig(Config& cfg);
void saveConfig(const Config& cfg);
