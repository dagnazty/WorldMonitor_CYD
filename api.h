#pragma once
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"

// ── User Agent ────────────────────────────────────────────────────────────
#define CHROME_UA "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36"

// ── HTTP Fetch ────────────────────────────────────────────────────────────

String httpGet(const String& url) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(client, url);
  http.addHeader("User-Agent", CHROME_UA);
  http.addHeader("Accept", "application/json");
  http.setTimeout(15000);

  Serial.printf("GET %s\n", url.c_str());
  int code = http.GET();
  String payload = "";

  if (code == HTTP_CODE_OK) {
    payload = http.getString();
    Serial.printf("  OK %d bytes\n", payload.length());
  } else {
    Serial.printf("  FAIL %d\n", code);
  }

  http.end();
  return payload;
}

// Helper: parse just price + prevClose + sparkline from Yahoo Finance chart JSON
// Uses a streaming approach — only extract what we need from the raw string
// to avoid allocating a huge JsonDocument for 30KB+ responses.

bool extractYahooNumber(const String& json, const char* key, float& out) {
  out = 0;

  int idx = json.indexOf(key);
  if (idx < 0) return false;

  int pos = idx + strlen(key);
  while (pos < (int)json.length() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) {
    pos++;
  }

  // Some Yahoo fields can be objects like {"raw":123.4,...}; prefer the raw value.
  if (pos < (int)json.length() && json[pos] == '{') {
    int rawIdx = json.indexOf("\"raw\":", pos);
    if (rawIdx < 0) return false;
    pos = rawIdx + 6;
  }

  while (pos < (int)json.length() && (json[pos] == ' ' || json[pos] == '"')) {
    pos++;
  }

  int end = pos;
  while (end < (int)json.length()) {
    char c = json[end];
    if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.') {
      end++;
      continue;
    }
    break;
  }

  if (end <= pos) return false;
  out = json.substring(pos, end).toFloat();
  return out != 0 || json.substring(pos, end) == "0" || json.substring(pos, end) == "0.0";
}

bool parseYahooQuote(const String& json, float& price, float& prevClose,
                     float* sparkline, int& sparkLen, int maxSpark) {
  price = 0;
  prevClose = 0;
  sparkLen = 0;

  // Extract regularMarketPrice from meta
  if (!extractYahooNumber(json, "\"regularMarketPrice\":", price) || price <= 0) return false;

  // Extract chartPreviousClose
  extractYahooNumber(json, "\"chartPreviousClose\":", prevClose);
  if (prevClose <= 0) extractYahooNumber(json, "\"previousClose\":", prevClose);
  if (prevClose <= 0) prevClose = price;

  // Extract sparkline from "close":[...] array
  int idx = json.indexOf("\"close\":[");
  if (idx >= 0 && maxSpark > 0 && sparkline != nullptr) {
    int start = idx + 9;
    int end = json.indexOf(']', start);
    if (end > start && end - start < 50000) {
      String closeStr = json.substring(start, end);
      // Count values to determine step size
      int count = 1;
      for (int i = 0; i < (int)closeStr.length(); i++) {
        if (closeStr[i] == ',') count++;
      }
      int step = max(1, count / maxSpark);

      int pos = 0;
      int valIdx = 0;
      while (pos < (int)closeStr.length() && sparkLen < maxSpark) {
        int next = closeStr.indexOf(',', pos);
        if (next < 0) next = closeStr.length();

        if (valIdx % step == 0) {
          String val = closeStr.substring(pos, next);
          val.trim();
          if (val != "null" && val.length() > 0) {
            sparkline[sparkLen++] = val.toFloat();
          }
        }

        pos = next + 1;
        valIdx++;
      }
    }
  }

  return true;
}

// ══════════════════════════════════════════════════════════════════════════
// MARKET DATA — Yahoo Finance (free, no key)
// ══════════════════════════════════════════════════════════════════════════

const char* MARKET_SYMBOLS[] = {
  "^GSPC", "^DJI", "^IXIC", "^RUT",
  "BTC-USD", "ETH-USD",
  "GC=F", "CL=F",
  "EURUSD=X", "^TNX"
};
const char* MARKET_DISPLAY[] = {
  "SPX", "DJI", "IXIC", "RUT",
  "BTC", "ETH",
  "GOLD", "OIL",
  "EUR", "10Y"
};
const char* MARKET_NAMES[] = {
  "S&P 500", "Dow Jones", "Nasdaq", "Russell 2000",
  "Bitcoin", "Ethereum",
  "Gold", "Crude Oil",
  "EUR/USD", "10Y Yield"
};
const int MARKET_SYMBOL_COUNT = 10;

void fetchMarketData(MarketData& mkt) {
  mkt.count = 0;

  for (int i = 0; i < MARKET_SYMBOL_COUNT && mkt.count < 20; i++) {
    String url = String("https://query1.finance.yahoo.com/v8/finance/chart/") + MARKET_SYMBOLS[i];
    String resp = httpGet(url);

    if (resp.length() == 0) continue;

    float price, prevClose;
    float spark[12];
    int sparkLen;

    if (!parseYahooQuote(resp, price, prevClose, spark, sparkLen, 12)) {
      Serial.printf("  %s: parse failed\n", MARKET_SYMBOLS[i]);
      continue;
    }

    Quote& q = mkt.quotes[mkt.count];
    strlcpy(q.symbol, MARKET_DISPLAY[i], sizeof(q.symbol));
    strlcpy(q.name, MARKET_NAMES[i], sizeof(q.name));
    q.price = price;
    q.change = (prevClose > 0) ? ((price - prevClose) / prevClose) * 100.0f : 0.0f;
    q.sparklineLen = sparkLen;
    for (int k = 0; k < sparkLen; k++) q.sparkline[k] = spark[k];

    mkt.count++;
    Serial.printf("  %s: %.2f (%+.2f%%) spark:%d\n", q.symbol, q.price, q.change, sparkLen);

    delay(200);
  }

  Serial.printf("Markets: %d quotes\n", mkt.count);
}

// ══════════════════════════════════════════════════════════════════════════
// FEAR & GREED — VIX-derived (CNN blocks ESP32 with 418)
// ══════════════════════════════════════════════════════════════════════════
// VIX → Fear/Greed mapping:
//   VIX < 12  → Extreme Greed (90)
//   VIX 12-16 → Greed (75)
//   VIX 16-20 → Neutral (50)
//   VIX 20-25 → Fear (30)
//   VIX 25-30 → High Fear (15)
//   VIX > 30  → Extreme Fear (5)

int vixToFearGreed(float vix) {
  if (vix <= 0) return 50;
  if (vix < 12) return 90;
  if (vix < 14) return 80;
  if (vix < 16) return 70;
  if (vix < 18) return 60;
  if (vix < 20) return 50;
  if (vix < 22) return 40;
  if (vix < 25) return 30;
  if (vix < 30) return 20;
  if (vix < 35) return 10;
  return 5;
}

const char* fgLabel(int score) {
  if (score >= 75) return "Extreme Greed";
  if (score >= 55) return "Greed";
  if (score >= 45) return "Neutral";
  if (score >= 25) return "Fear";
  return "Extreme Fear";
}

void fetchFearGreed(FearGreed& fg) {
  memset(&fg, 0, sizeof(fg));
  fg.compositeScore = 50;
  strlcpy(fg.compositeLabel, "N/A", sizeof(fg.compositeLabel));
  strlcpy(fg.fedRate, "N/A", sizeof(fg.fedRate));

  // VIX
  String vixResp = httpGet("https://query1.finance.yahoo.com/v8/finance/chart/^VIX");
  if (vixResp.length() > 0) {
    float price, prev;
    float sp[1]; int sl;
    if (parseYahooQuote(vixResp, price, prev, sp, sl, 0)) {
      fg.vix = price;
      fg.compositeScore = vixToFearGreed(price);
      fg.cnnFearGreed = fg.compositeScore;
      strlcpy(fg.compositeLabel, fgLabel(fg.compositeScore), sizeof(fg.compositeLabel));
      Serial.printf("  VIX: %.1f -> F&G: %d (%s)\n", fg.vix, fg.compositeScore, fg.compositeLabel);
    }
  }

  delay(200);

  // 10Y yield
  String tnxResp = httpGet("https://query1.finance.yahoo.com/v8/finance/chart/^TNX");
  if (tnxResp.length() > 0) {
    float price, prev;
    float sp[1]; int sl;
    if (parseYahooQuote(tnxResp, price, prev, sp, sl, 0)) {
      fg.yield10y = price;
      Serial.printf("  10Y: %.2f\n", fg.yield10y);
    }
  }
}

// ══════════════════════════════════════════════════════════════════════════
// NATURAL EVENTS — USGS Earthquakes + NASA EONET (free, no key)
// ══════════════════════════════════════════════════════════════════════════

void fetchNaturalEvents(NaturalEvents& nat) {
  nat.count = 0;

  // ── USGS Earthquakes (M4.5+ last 24h) ────────────────────────────────
  String eqResp = httpGet("https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/4.5_day.geojson");

  if (eqResp.length() > 0) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, eqResp);
    if (!err) {
      JsonArray features = doc["features"];
      for (JsonObject f : features) {
        if (nat.count >= 10) break;

        NaturalEvent& e = nat.items[nat.count];
        const char* place = f["properties"]["place"] | "Unknown";
        float mag = f["properties"]["mag"] | 0.0f;

        char title[100];
        snprintf(title, sizeof(title), "M%.1f - %s", mag, place);
        strlcpy(e.title, title, sizeof(e.title));
        strlcpy(e.category, "earthquakes", sizeof(e.category));
        strlcpy(e.categoryTitle, "Earthquakes", sizeof(e.categoryTitle));

        e.lon = f["geometry"]["coordinates"][0] | 0.0f;
        e.lat = f["geometry"]["coordinates"][1] | 0.0f;
        e.magnitude = mag;
        strlcpy(e.magnitudeUnit, "Mw", sizeof(e.magnitudeUnit));
        strlcpy(e.sourceName, "USGS", sizeof(e.sourceName));
        e.date = f["properties"]["time"] | 0UL;

        e.stormName[0] = '\0';
        e.windKt = 0;
        e.pressureMb = 0;
        e.stormCategory = -1;

        nat.count++;
      }
      Serial.printf("  Earthquakes: %d\n", nat.count);
    } else {
      Serial.printf("  EQ parse error: %s\n", err.c_str());
    }
  }

  delay(300);

  // ── NASA EONET (volcanoes, wildfires, storms) ─────────────────────────
  String eoResp = httpGet("https://eonet.gsfc.nasa.gov/api/v3/events?status=open&days=7&limit=15");

  if (eoResp.length() > 0) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, eoResp);
    if (!err) {
      JsonArray events = doc["events"];
      for (JsonObject ev : events) {
        if (nat.count >= 20) break;

        NaturalEvent& e = nat.items[nat.count];
        strlcpy(e.title, ev["title"] | "Unknown", sizeof(e.title));

        const char* catId = ev["categories"][0]["id"] | "unknown";
        const char* catTitle = ev["categories"][0]["title"] | catId;
        strlcpy(e.category, catId, sizeof(e.category));
        strlcpy(e.categoryTitle, catTitle, sizeof(e.categoryTitle));

        JsonArray geom = ev["geometry"];
        if (geom.size() > 0) {
          JsonObject latest = geom[geom.size() - 1];
          e.lon = latest["coordinates"][0] | 0.0f;
          e.lat = latest["coordinates"][1] | 0.0f;
          e.magnitude = latest["magnitudeValue"] | 0.0f;
          strlcpy(e.magnitudeUnit, latest["magnitudeUnit"] | "", sizeof(e.magnitudeUnit));
        } else {
          e.lat = 0; e.lon = 0; e.magnitude = 0;
          e.magnitudeUnit[0] = '\0';
        }

        const char* srcId = ev["sources"][0]["id"] | "";
        strlcpy(e.sourceName, srcId, sizeof(e.sourceName));

        e.date = 0;
        e.stormName[0] = '\0';
        e.windKt = 0;
        e.pressureMb = 0;
        e.stormCategory = -1;

        nat.count++;
      }
    } else {
      Serial.printf("  EONET parse error: %s\n", err.c_str());
    }
  }

  Serial.printf("Natural events: %d total\n", nat.count);
}
