#pragma once
#include <TFT_eSPI.h>
#include "config.h"
#include "ui.h"

// ── Helpers ───────────────────────────────────────────────────────────────

const Quote* findQuoteBySymbol(const MarketData& mkt, const char* symbol) {
  for (int i = 0; i < mkt.count; i++) {
    if (strcmp(mkt.quotes[i].symbol, symbol) == 0) return &mkt.quotes[i];
  }
  return nullptr;
}

const Quote* findSimpleQuote(const MarketData& mkt, int slot) {
  static const char* preferred[] = { "SPX", "BTC", "OIL", "10Y" };
  if (slot >= 0 && slot < 4) {
    const Quote* q = findQuoteBySymbol(mkt, preferred[slot]);
    if (q) return q;
  }
  if (slot >= 0 && slot < mkt.count) return &mkt.quotes[slot];
  return nullptr;
}

const NewsItem* findTopAlertNews(const NewsDigest& news) {
  const NewsItem* best = nullptr;
  for (int i = 0; i < news.count; i++) {
    const NewsItem& item = news.items[i];
    if (!best || item.threatLevel > best->threatLevel || (item.isAlert && !best->isAlert)) {
      best = &item;
    }
  }
  return best;
}

const char* alertLevelLabel(int level) {
  switch (level) {
    case 4: return "CRITICAL";
    case 3: return "HIGH";
    case 2: return "ELEVATED";
    case 1: return "LOW";
    default: return "STABLE";
  }
}

uint16_t alertLevelColor(int level) {
  switch (level) {
    case 4: return COLOR_RED;
    case 3: return COLOR_ORANGE;
    case 2: return COLOR_YELLOW;
    case 1: return COLOR_GREEN;
    default: return COLOR_ACCENT;
  }
}

void drawSimpleCardLabel(TFT_eSPI& tft, const char* label, int x, int y, uint16_t bg) {
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED, bg);
  drawString(tft, label, x, y, TL_DATUM);
}

void drawSimpleQuoteCard(TFT_eSPI& tft, int x, int y, int w, int h, const Quote* q) {
  drawPanel(tft, x, y, w, h, COLOR_PANEL, COLOR_BORDER);
  if (!q) {
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
    drawString(tft, "NO DATA", x + 8, y + h / 2 - 4, TL_DATUM);
    return;
  }

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  drawString(tft, q->symbol, x + 8, y + 5, TL_DATUM);

  char pBuf[16];
  if (q->price >= 10000) snprintf(pBuf, sizeof(pBuf), "%.0f", q->price);
  else if (q->price >= 100) snprintf(pBuf, sizeof(pBuf), "%.1f", q->price);
  else snprintf(pBuf, sizeof(pBuf), "%.2f", q->price);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, pBuf, x + 8, y + 18, TL_DATUM);

  char cBuf[12];
  snprintf(cBuf, sizeof(cBuf), "%+.1f%%", q->change);
  tft.setTextColor(changeColor(q->change), COLOR_PANEL);
  drawString(tft, cBuf, x + 8, y + 40, TL_DATUM);
}

// Usable content height (above nav bar)
#define CYD_CONTENT_BOTTOM (SCREEN_H - NAV_BAR_H - 2)

// ══════════════════════════════════════════════════════════════════════════
// SIMPLE UI
// ══════════════════════════════════════════════════════════════════════════

void drawSimpleSummaryScreen(TFT_eSPI& tft, const MarketData& mkt, const FearGreed& fg,
                             const RiskScores& risk, bool wifiOK, int alertLevel) {
  drawHeader(tft, "SUMMARY", COLOR_ACCENT);

  int y = CONTENT_Y + 2;

  // Fear & Greed panel
  drawPanel(tft, 2, y, SCREEN_W - 4, 60, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "FEAR & GREED", 8, y + 5, COLOR_PANEL);
  if (!wifiOK) drawBadge(tft, SCREEN_W - 70, y + 4, "OFFLINE", COLOR_RED_DIM, COLOR_RED);

  char fgBuf[8];
  snprintf(fgBuf, sizeof(fgBuf), "%d", fg.compositeScore);
  tft.setTextSize(4);
  tft.setTextColor(fgColor(fg.compositeScore), COLOR_PANEL);
  drawString(tft, fgBuf, 10, y + 18, TL_DATUM);

  tft.setTextSize(2);
  tft.setTextColor(fgColor(fg.compositeScore), COLOR_PANEL);
  drawTruncated(tft, fg.compositeLabel, 94, y + 22, 152);

  char subBuf[32];
  snprintf(subBuf, sizeof(subBuf), "VIX %.1f  10Y %.2f", fg.vix, fg.yield10y);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
  drawString(tft, subBuf, 94, y + 46, TL_DATUM);

  y += 64;

  // Risk + Alert row
  drawPanel(tft, 2, y, 156, 40, COLOR_PANEL, COLOR_BORDER);
  drawPanel(tft, 162, y, 156, 40, COLOR_PANEL, COLOR_BORDER);

  drawSimpleCardLabel(tft, "TOP RISK", 8, y + 5, COLOR_PANEL);
  if (risk.count > 0) {
    const RiskItem& topRisk = risk.items[0];
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    drawTruncated(tft, topRisk.region, 8, y + 16, 100);
    char riskBuf[8];
    snprintf(riskBuf, sizeof(riskBuf), "%d", topRisk.combinedScore);
    tft.setTextColor(riskColor(topRisk.combinedScore), COLOR_PANEL);
    drawString(tft, riskBuf, 150, y + 16, TR_DATUM);
  }

  drawSimpleCardLabel(tft, "ALERT LEVEL", 168, y + 5, COLOR_PANEL);
  tft.setTextSize(2);
  tft.setTextColor(alertLevelColor(alertLevel), COLOR_PANEL);
  drawTruncated(tft, alertLevelLabel(alertLevel), 168, y + 16, 132);

  y += 44;

  // Quote cards
  const Quote* q0 = findSimpleQuote(mkt, 0);
  const Quote* q1 = findSimpleQuote(mkt, 1);
  const Quote* q2 = findSimpleQuote(mkt, 2);
  int cardH = CYD_CONTENT_BOTTOM - y - 4;
  if (cardH > 66) cardH = 66;
  if (cardH < 30) cardH = 30;
  drawSimpleQuoteCard(tft, 2, y, 102, cardH, q0);
  drawSimpleQuoteCard(tft, 109, y, 102, cardH, q1);
  drawSimpleQuoteCard(tft, 216, y, 102, cardH, q2);

  drawScreenIndicator(tft, 0, 4);
}

void drawSimpleMarketsScreen(TFT_eSPI& tft, const MarketData& mkt, bool wifiOK,
                             int& scrollOff, int& maxScroll) {
  drawHeader(tft, "MARKETS", COLOR_GREEN);

  int y = CONTENT_Y + 2;
  int rowH = 44;
  int visibleRows = (CYD_CONTENT_BOTTOM - y) / rowH;
  maxScroll = max(0, mkt.count - visibleRows);
  if (scrollOff > maxScroll) scrollOff = maxScroll;
  if (scrollOff < 0) scrollOff = 0;

  if (mkt.count <= 0) {
    drawPanel(tft, 2, y, SCREEN_W - 4, 60, COLOR_PANEL, COLOR_BORDER);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    drawString(tft, wifiOK ? "Loading..." : "Offline", 8, y + 18, TL_DATUM);
    drawScreenIndicator(tft, 1, 4);
    return;
  }

  for (int idx = scrollOff; idx < mkt.count && y < CYD_CONTENT_BOTTOM - rowH + 2; idx++) {
    const Quote& q = mkt.quotes[idx];
    drawPanel(tft, 2, y, SCREEN_W - 4, rowH - 2, COLOR_PANEL, COLOR_BORDER);
    tft.fillRect(2, y, 4, rowH - 2, changeColor(q.change));

    tft.setTextSize(2);
    tft.setTextColor(COLOR_ACCENT, COLOR_PANEL);
    drawString(tft, q.symbol, 10, y + 5, TL_DATUM);

    char cBuf[12];
    snprintf(cBuf, sizeof(cBuf), "%+.1f%%", q.change);
    tft.setTextColor(changeColor(q.change), COLOR_PANEL);
    drawString(tft, cBuf, SCREEN_W - 10, y + 5, TR_DATUM);

    char pBuf[16];
    if (q.price >= 10000) snprintf(pBuf, sizeof(pBuf), "%.0f", q.price);
    else if (q.price >= 100) snprintf(pBuf, sizeof(pBuf), "%.1f", q.price);
    else snprintf(pBuf, sizeof(pBuf), "%.2f", q.price);
    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    drawString(tft, pBuf, 10, y + 24, TL_DATUM);

    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
    drawTruncated(tft, q.name, 118, y + 28, 190);

    y += rowH;
  }

  char sBuf[24];
  snprintf(sBuf, sizeof(sBuf), "%d/%d", min(scrollOff + 1, max(1, mkt.count)), max(1, mkt.count));
  drawStatusBar(tft, sBuf, COLOR_TEXT_MUTED);
  drawScreenIndicator(tft, 1, 4);
}

void drawSimpleAlertsScreen(TFT_eSPI& tft, const RiskScores& risk, const NewsDigest& news,
                            const NaturalEvents& events, int alertLevel) {
  drawHeader(tft, "WATCHLIST", COLOR_ORANGE);

  int y = CONTENT_Y + 2;

  // Alert level
  drawPanel(tft, 2, y, SCREEN_W - 4, 34, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "SYSTEM ALERT", 8, y + 5, COLOR_PANEL);
  tft.setTextSize(2);
  tft.setTextColor(alertLevelColor(alertLevel), COLOR_PANEL);
  drawString(tft, alertLevelLabel(alertLevel), SCREEN_W - 10, y + 12, TR_DATUM);
  y += 38;

  // Top risk
  drawPanel(tft, 2, y, SCREEN_W - 4, 38, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "TOP RISK", 8, y + 5, COLOR_PANEL);
  if (risk.count > 0) {
    const RiskItem& topRisk = risk.items[0];
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    drawTruncated(tft, topRisk.region, 8, y + 16, 210);
    char scoreBuf[8];
    snprintf(scoreBuf, sizeof(scoreBuf), "%d", topRisk.combinedScore);
    tft.setTextColor(riskColor(topRisk.combinedScore), COLOR_PANEL);
    drawString(tft, scoreBuf, SCREEN_W - 10, y + 16, TR_DATUM);
  }
  y += 42;

  // Top news
  drawPanel(tft, 2, y, SCREEN_W - 4, 44, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "NEWS", 8, y + 5, COLOR_PANEL);
  const NewsItem* topNews = findTopAlertNews(news);
  if (topNews) {
    tft.setTextSize(1);
    tft.setTextColor(threatColor(topNews->threatLevel), COLOR_PANEL);
    drawTruncated(tft, topNews->title, 8, y + 16, 300);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
    char metaBuf[48];
    snprintf(metaBuf, sizeof(metaBuf), "%s  %s", topNews->source, topNews->region);
    drawString(tft, metaBuf, 8, y + 30, TL_DATUM);
  }
  y += 48;

  // Top event
  drawPanel(tft, 2, y, SCREEN_W - 4, 44, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "EVENT", 8, y + 5, COLOR_PANEL);
  if (events.count > 0) {
    const NaturalEvent& event = events.items[0];
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    if (strlen(event.stormName) > 0) drawTruncated(tft, event.stormName, 8, y + 16, 300);
    else drawTruncated(tft, event.title, 8, y + 16, 300);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
    drawString(tft, event.categoryTitle, 8, y + 30, TL_DATUM);
  }

  drawScreenIndicator(tft, 2, 4);
}

void drawSimpleSettingsScreen(TFT_eSPI& tft, const Config& cfg, bool wifiOK) {
  drawHeader(tft, "SETTINGS", COLOR_TEXT_DIM);

  int y = CONTENT_Y + 2;

  // Network
  drawPanel(tft, 2, y, SCREEN_W - 4, 40, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "NETWORK", 8, y + 5, COLOR_PANEL);
  drawBadge(tft, SCREEN_W - 80, y + 4, wifiOK ? "ONLINE" : "OFFLINE",
            wifiOK ? COLOR_GREEN_DIM : COLOR_RED_DIM,
            wifiOK ? COLOR_GREEN : COLOR_RED);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawTruncated(tft, cfg.wifiSSID, 8, y + 18, 200);
  y += 44;

  // Mode
  drawPanel(tft, 2, y, SCREEN_W - 4, 38, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "MODE", 8, y + 5, COLOR_PANEL);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_ACCENT, COLOR_PANEL);
  drawString(tft, isSimpleUiMode(cfg) ? "SIMPLE" : "CLASSIC", 8, y + 16, TL_DATUM);
  drawBadge(tft, SCREEN_W - 100, y + 4, "TAP TO TOGGLE", COLOR_ACCENT_DIM, COLOR_ACCENT);

  char refreshBuf[20];
  snprintf(refreshBuf, sizeof(refreshBuf), "%ds", cfg.refreshSeconds);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, refreshBuf, SCREEN_W - 10, y + 16, TR_DATUM);
  y += 42;

  // System
  drawPanel(tft, 2, y, SCREEN_W - 4, 38, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "SYSTEM", 8, y + 5, COLOR_PANEL);
  char heapBuf[24];
  snprintf(heapBuf, sizeof(heapBuf), "%luKB free", (unsigned long)ESP.getFreeHeap() / 1024);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, heapBuf, 8, y + 16, TL_DATUM);
  y += 42;

  // Controls hint
  drawPanel(tft, 2, y, SCREEN_W - 4, 34, COLOR_PANEL, COLOR_BORDER);
  drawSimpleCardLabel(tft, "NAVIGATION", 8, y + 5, COLOR_PANEL);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, "Use bottom buttons or tap center", 8, y + 18, TL_DATUM);

  drawScreenIndicator(tft, 3, 4);
}

// ══════════════════════════════════════════════════════════════════════════
// CLASSIC UI
// ══════════════════════════════════════════════════════════════════════════

void drawDashboardScreen(TFT_eSPI& tft, const MarketData& mkt, const FearGreed& fg, const RiskScores& risk) {
  drawHeader(tft, "DASHBOARD", COLOR_ACCENT);

  // Left: Fear & Greed panel
  drawPanel(tft, 2, CONTENT_Y + 1, 155, 80, COLOR_PANEL, COLOR_BORDER);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  drawString(tft, "FEAR & GREED", 8, CONTENT_Y + 5, TL_DATUM);

  int gaugeX = 45, gaugeY = CONTENT_Y + 56;
  drawGauge(tft, gaugeX, gaugeY, 24, fg.compositeScore, fgColor(fg.compositeScore));

  char scoreBuf[8];
  snprintf(scoreBuf, sizeof(scoreBuf), "%d", fg.compositeScore);
  tft.setTextSize(2);
  tft.setTextColor(fgColor(fg.compositeScore), COLOR_PANEL);
  drawString(tft, scoreBuf, gaugeX, gaugeY - 10, MC_DATUM);

  tft.setTextSize(1);
  tft.setTextColor(fgColor(fg.compositeScore), COLOR_PANEL);
  drawString(tft, fg.compositeLabel, 85, CONTENT_Y + 28, TL_DATUM);

  tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
  char vBuf[16]; snprintf(vBuf, sizeof(vBuf), "VIX  %.1f", fg.vix);
  drawString(tft, vBuf, 85, CONTENT_Y + 42, TL_DATUM);
  char yBuf[16]; snprintf(yBuf, sizeof(yBuf), "10Y  %.2f%%", fg.yield10y);
  drawString(tft, yBuf, 85, CONTENT_Y + 54, TL_DATUM);

  // Right: Top Risks
  drawPanel(tft, 161, CONTENT_Y + 1, 157, 80, COLOR_PANEL, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  drawString(tft, "TOP RISKS", 167, CONTENT_Y + 5, TL_DATUM);

  int ry = CONTENT_Y + 15;
  int risksToShow = min(5, risk.count);
  for (int i = 0; i < risksToShow; i++) {
    const RiskItem& r = risk.items[i];
    tft.fillRect(167, ry, 3, 11, riskColor(r.combinedScore));
    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    drawTruncated(tft, r.region, 174, ry + 1, 75);
    char sBuf[8]; snprintf(sBuf, sizeof(sBuf), "%d", r.combinedScore);
    tft.setTextColor(riskColor(r.combinedScore), COLOR_PANEL);
    drawString(tft, sBuf, 310, ry + 1, TR_DATUM);
    ry += 13;
  }

  // Bottom: Market ticker
  int my = CONTENT_Y + 84;
  drawPanel(tft, 2, my, 316, CYD_CONTENT_BOTTOM - my - 2, COLOR_PANEL, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  drawString(tft, "MARKETS", 8, my + 4, TL_DATUM);

  int mx = 8, mRow = my + 16;
  int quotesToShow = min(5, mkt.count);
  for (int i = 0; i < quotesToShow; i++) {
    const Quote& q = mkt.quotes[i];
    int colW = 62;
    tft.setTextSize(1);
    tft.setTextColor(COLOR_ACCENT, COLOR_PANEL);
    drawString(tft, q.symbol, mx, mRow, TL_DATUM);
    char pBuf[12];
    if (q.price >= 1000) snprintf(pBuf, sizeof(pBuf), "%.0f", q.price);
    else snprintf(pBuf, sizeof(pBuf), "%.2f", q.price);
    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    drawString(tft, pBuf, mx, mRow + 12, TL_DATUM);
    char cBuf[10]; snprintf(cBuf, sizeof(cBuf), "%+.1f%%", q.change);
    tft.setTextColor(changeColor(q.change), COLOR_PANEL);
    drawString(tft, cBuf, mx, mRow + 24, TL_DATUM);
    if (q.sparklineLen > 1) {
      float spark[12]; memcpy(spark, q.sparkline, q.sparklineLen * sizeof(float));
      drawSparkline(tft, mx, mRow + 36, colW - 4, 14, spark, q.sparklineLen);
    }
    mx += colW;
  }

  drawScreenIndicator(tft, 0, 7);
}

void drawMarketsScreen(TFT_eSPI& tft, const MarketData& mkt, const FearGreed& fg,
                       int& scrollOff, int& maxScroll) {
  drawHeader(tft, "MARKETS", COLOR_GREEN);

  int y = CONTENT_Y + 2;
  char fgBuf[32]; snprintf(fgBuf, sizeof(fgBuf), "F&G %d", fg.compositeScore);
  drawBadge(tft, MARGIN, y, fgBuf, fgColor(fg.compositeScore) & 0x7BEF, COLOR_TEXT);
  char vBuf[16]; snprintf(vBuf, sizeof(vBuf), "VIX %.1f", fg.vix);
  drawBadge(tft, MARGIN + 48, y, vBuf, COLOR_PANEL, COLOR_TEXT_DIM);
  y += 14;
  tft.drawFastHLine(MARGIN, y, SCREEN_W - 2 * MARGIN, COLOR_BORDER);
  y += 3;

  int rowH = 18;
  int visibleRows = (CYD_CONTENT_BOTTOM - y) / rowH;
  maxScroll = max(0, mkt.count - visibleRows);

  for (int idx = scrollOff; idx < mkt.count && y < CYD_CONTENT_BOTTOM - rowH; idx++) {
    const Quote& q = mkt.quotes[idx];
    if ((idx - scrollOff) % 2 == 0) tft.fillRect(2, y - 1, SCREEN_W - 4, rowH, COLOR_PANEL);
    tft.fillRect(2, y - 1, 2, rowH, changeColor(q.change));

    tft.setTextSize(1);
    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    drawString(tft, q.symbol, 8, y + 2, TL_DATUM);
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    drawTruncated(tft, q.name, 52, y + 2, 60);

    char pBuf[16];
    if (q.price >= 10000) snprintf(pBuf, sizeof(pBuf), "%.0f", q.price);
    else if (q.price >= 100) snprintf(pBuf, sizeof(pBuf), "%.1f", q.price);
    else snprintf(pBuf, sizeof(pBuf), "%.2f", q.price);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    drawString(tft, pBuf, 172, y + 2, TR_DATUM);

    char cBuf[12]; snprintf(cBuf, sizeof(cBuf), "%+.2f%%", q.change);
    tft.setTextColor(changeColor(q.change), COLOR_BG);
    drawString(tft, cBuf, 225, y + 2, TR_DATUM);

    if (q.sparklineLen > 1) {
      float spark[12]; memcpy(spark, q.sparkline, q.sparklineLen * sizeof(float));
      drawSparkline(tft, 232, y + 1, 82, rowH - 3, spark, q.sparklineLen);
    }
    y += rowH;
  }

  if (maxScroll > 0) {
    char sBuf[16]; snprintf(sBuf, sizeof(sBuf), "%d/%d", scrollOff + 1, mkt.count);
    drawStatusBar(tft, sBuf, COLOR_TEXT_MUTED);
  }
  drawScreenIndicator(tft, 1, 7);
}

void drawRiskScreen(TFT_eSPI& tft, const RiskScores& risk, int& scrollOff, int& maxScroll) {
  drawHeader(tft, "REGIONAL RISK", COLOR_RED);
  int y = CONTENT_Y + 2;
  int rowH = 26;
  int visibleLines = (CYD_CONTENT_BOTTOM - y) / rowH;
  maxScroll = max(0, risk.count - visibleLines);

  tft.setTextSize(1);
  for (int idx = scrollOff; idx < risk.count && y < CYD_CONTENT_BOTTOM - rowH; idx++) {
    const RiskItem& r = risk.items[idx];
    drawPanel(tft, 2, y, SCREEN_W - 4, rowH - 2, COLOR_PANEL, COLOR_BORDER);
    tft.fillRect(2, y, 4, rowH - 2, riskColor(r.combinedScore));
    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    drawTruncated(tft, r.region, 10, y + 3, 90);
    char sBuf[8]; snprintf(sBuf, sizeof(sBuf), "%d", r.combinedScore);
    tft.setTextColor(riskColor(r.combinedScore), COLOR_PANEL);
    drawString(tft, sBuf, 108, y + 3, TL_DATUM);
    tft.setTextColor(trendColor(r.trend), COLOR_PANEL);
    drawString(tft, trendArrow(r.trend), 128, y + 3, TL_DATUM);
    drawBar(tft, 140, y + 4, SCREEN_W - 148, 8, r.combinedScore, 100, riskColor(r.combinedScore));
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
    char compBuf[48]; snprintf(compBuf, sizeof(compBuf), "News %d  Mil %d", r.newsActivity, r.militaryActivity);
    drawString(tft, compBuf, 10, y + 15, TL_DATUM);
    y += rowH;
  }
  if (risk.count == 0) {
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    drawString(tft, "No risk data", SCREEN_W / 2, SCREEN_H / 2, MC_DATUM);
  }
  drawScreenIndicator(tft, 2, 7);
}

void drawNewsScreen(TFT_eSPI& tft, const NewsDigest& news, int& scrollOff, int& maxScroll) {
  drawHeader(tft, "NEWS FEED", COLOR_ORANGE);
  int y = CONTENT_Y + 2;

  if (strlen(news.worldBrief) > 0 && scrollOff == 0) {
    drawPanel(tft, 2, y, SCREEN_W - 4, 16, COLOR_ACCENT_DIM, COLOR_ACCENT);
    tft.setTextSize(1);
    tft.setTextColor(COLOR_TEXT, COLOR_ACCENT_DIM);
    drawTruncated(tft, news.worldBrief, 6, y + 3, SCREEN_W - 12);
    y += 18;
  }

  int rowH = 24;
  int visibleLines = (CYD_CONTENT_BOTTOM - y) / rowH;
  maxScroll = max(0, news.count - visibleLines);
  tft.setTextSize(1);

  for (int idx = scrollOff; idx < news.count && y < CYD_CONTENT_BOTTOM - rowH; idx++) {
    const NewsItem& n = news.items[idx];
    uint16_t tColor = threatColor(n.threatLevel);
    tft.fillRect(2, y, 3, rowH - 2, tColor);
    if (n.isAlert) {
      tft.fillCircle(10, y + 5, 3, COLOR_RED);
      tft.setTextColor(COLOR_TEXT, COLOR_RED);
      drawString(tft, "!", 8, y + 1, TL_DATUM);
    }
    int textX = n.isAlert ? 18 : 10;
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    drawTruncated(tft, n.title, textX, y + 1, SCREEN_W - textX - MARGIN);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    char metaBuf[64];
    if (strlen(n.region) > 0) snprintf(metaBuf, sizeof(metaBuf), "%s  %s", n.source, n.region);
    else snprintf(metaBuf, sizeof(metaBuf), "%s", n.source);
    drawString(tft, metaBuf, textX, y + 12, TL_DATUM);
    y += rowH;
  }

  if (maxScroll > 0) {
    char sBuf[16]; snprintf(sBuf, sizeof(sBuf), "%d/%d", scrollOff + 1, news.count);
    drawStatusBar(tft, sBuf, COLOR_TEXT_MUTED);
  }
  drawScreenIndicator(tft, 3, 7);
}

void drawNaturalScreen(TFT_eSPI& tft, const NaturalEvents& events, int& scrollOff, int& maxScroll) {
  drawHeader(tft, "NATURAL EVENTS", COLOR_YELLOW);
  int y = CONTENT_Y + 2;
  int rowH = 28;
  int visibleLines = (CYD_CONTENT_BOTTOM - y) / rowH;
  maxScroll = max(0, events.count - visibleLines);
  tft.setTextSize(1);

  for (int idx = scrollOff; idx < events.count && y < CYD_CONTENT_BOTTOM - rowH; idx++) {
    const NaturalEvent& e = events.items[idx];
    const char* icon = "??";
    uint16_t iconBg = COLOR_TEXT_MUTED;
    if (strcmp(e.category, "earthquakes") == 0)      { icon = "EQ"; iconBg = COLOR_RED; }
    else if (strcmp(e.category, "volcanoes") == 0)    { icon = "VO"; iconBg = COLOR_ORANGE; }
    else if (strcmp(e.category, "wildfires") == 0)    { icon = "FI"; iconBg = COLOR_ORANGE; }
    else if (strcmp(e.category, "severeStorms") == 0) { icon = "ST"; iconBg = COLOR_BLUE; }
    else if (strcmp(e.category, "floods") == 0)       { icon = "FL"; iconBg = COLOR_BLUE; }

    drawBadge(tft, 4, y + 1, icon, iconBg);
    tft.setTextColor(COLOR_TEXT, COLOR_BG);
    if (strlen(e.stormName) > 0) drawTruncated(tft, e.stormName, 24, y + 1, SCREEN_W - 28);
    else drawTruncated(tft, e.title, 24, y + 1, SCREEN_W - 28);

    char detBuf[64];
    if (e.magnitude > 0.01f) snprintf(detBuf, sizeof(detBuf), "%.1f%s  %.1f,%.1f", e.magnitude, e.magnitudeUnit, e.lat, e.lon);
    else snprintf(detBuf, sizeof(detBuf), "%.1f, %.1f", e.lat, e.lon);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    drawString(tft, detBuf, 24, y + 13, TL_DATUM);

    tft.drawFastHLine(24, y + rowH - 2, SCREEN_W - 28, COLOR_BORDER);
    y += rowH;
  }

  if (maxScroll > 0) {
    char sBuf[16]; snprintf(sBuf, sizeof(sBuf), "%d/%d", scrollOff + 1, events.count);
    drawStatusBar(tft, sBuf, COLOR_TEXT_MUTED);
  }
  drawScreenIndicator(tft, 4, 7);
}

void drawForecastScreen(TFT_eSPI& tft, const Forecasts& fc, int& scrollOff, int& maxScroll) {
  drawHeader(tft, "FORECASTS", COLOR_PURPLE);
  int y = CONTENT_Y + 2;
  int rowH = 36;
  int visibleLines = (CYD_CONTENT_BOTTOM - y) / rowH;
  maxScroll = max(0, fc.count - visibleLines);
  tft.setTextSize(1);

  for (int idx = scrollOff; idx < fc.count && y < CYD_CONTENT_BOTTOM - rowH; idx++) {
    const Forecast& f = fc.items[idx];
    drawPanel(tft, 2, y, SCREEN_W - 4, rowH - 2, COLOR_PANEL, COLOR_BORDER);

    int probPct = (int)(f.probability * 100);
    char probBuf[8]; snprintf(probBuf, sizeof(probBuf), "%d%%", probPct);
    uint16_t probColor = COLOR_GREEN;
    if (probPct >= 60) probColor = COLOR_RED;
    else if (probPct >= 40) probColor = COLOR_ORANGE;
    else if (probPct >= 25) probColor = COLOR_YELLOW;

    tft.fillCircle(20, y + rowH / 2 - 1, 14, COLOR_CARD);
    tft.drawCircle(20, y + rowH / 2 - 1, 14, probColor);
    tft.setTextColor(probColor, COLOR_CARD);
    drawString(tft, probBuf, 20, y + rowH / 2 - 1, MC_DATUM);

    tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
    drawTruncated(tft, f.title, 40, y + 3, SCREEN_W - 48);
    drawBar(tft, 40, y + 15, 120, 5, f.probability, 1.0f, probColor);

    float delta = f.probability - f.priorProbability;
    if (delta > 0.005f || delta < -0.005f) {
      char dBuf[8]; snprintf(dBuf, sizeof(dBuf), "%+.0f%%", delta * 100);
      drawBadge(tft, 165, y + 13, dBuf, delta > 0 ? COLOR_RED_DIM : COLOR_GREEN_DIM,
                delta > 0 ? COLOR_RED : COLOR_GREEN);
    }

    char metaBuf[32]; snprintf(metaBuf, sizeof(metaBuf), "%d%% conf  %s", (int)(f.confidence * 100), f.timeHorizon);
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
    drawString(tft, metaBuf, 40, y + 24, TL_DATUM);
    tft.setTextColor(trendColor(f.trend), COLOR_PANEL);
    drawString(tft, trendArrow(f.trend), 140, y + 24, TL_DATUM);

    drawBadge(tft, SCREEN_W - MARGIN - textPixelWidth(tft, f.domain) - 10, y + 23, f.domain, COLOR_CARD, COLOR_PURPLE);
    y += rowH;
  }

  if (fc.count == 0) {
    tft.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    drawString(tft, "No forecast data", SCREEN_W / 2, SCREEN_H / 2, MC_DATUM);
  }
  drawScreenIndicator(tft, 5, 7);
}

void drawSettingsScreen(TFT_eSPI& tft, const Config& cfg, bool wifiOK, int scrollOff) {
  drawHeader(tft, "SETTINGS", COLOR_TEXT_DIM);
  int y = CONTENT_Y + 4;
  tft.setTextSize(1);

  // WiFi card
  drawPanel(tft, 2, y, SCREEN_W - 4, 42, COLOR_PANEL, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  drawString(tft, "NETWORK", 8, y + 3, TL_DATUM);

  drawBadge(tft, SCREEN_W - 70, y + 2, wifiOK ? "CONNECTED" : "OFFLINE",
            wifiOK ? COLOR_GREEN_DIM : COLOR_RED_DIM,
            wifiOK ? COLOR_GREEN : COLOR_RED);

  tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
  drawString(tft, "SSID", 8, y + 16, TL_DATUM);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, cfg.wifiSSID, 50, y + 16, TL_DATUM);

  if (wifiOK) {
    tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
    drawString(tft, "IP", 8, y + 28, TL_DATUM);
    tft.setTextColor(COLOR_ACCENT, COLOR_PANEL);
    drawString(tft, WiFi.localIP().toString().c_str(), 50, y + 28, TL_DATUM);
  }
  y += 46;

  // Config card
  drawPanel(tft, 2, y, SCREEN_W - 4, 42, COLOR_PANEL, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  drawString(tft, "CONFIG", 8, y + 3, TL_DATUM);

  tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
  drawString(tft, "Refresh", 8, y + 16, TL_DATUM);
  char refBuf[8]; snprintf(refBuf, sizeof(refBuf), "%ds", cfg.refreshSeconds);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, refBuf, 60, y + 16, TL_DATUM);

  tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
  drawString(tft, "Mode", 110, y + 16, TL_DATUM);
  drawBadge(tft, 150, y + 15, isSimpleUiMode(cfg) ? "SIMPLE" : "CLASSIC", COLOR_ACCENT_DIM, COLOR_ACCENT);

  tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
  drawString(tft, "UI", 8, y + 28, TL_DATUM);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, isSimpleUiMode(cfg) ? "Simple (4 screens)" : "Classic (7 screens)", 30, y + 28, TL_DATUM);
  y += 46;

  // System card
  drawPanel(tft, 2, y, SCREEN_W - 4, 42, COLOR_PANEL, COLOR_BORDER);
  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  drawString(tft, "SYSTEM", 8, y + 3, TL_DATUM);

  char heapBuf[24];
  snprintf(heapBuf, sizeof(heapBuf), "%luKB free", (unsigned long)ESP.getFreeHeap() / 1024);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, heapBuf, 8, y + 16, TL_DATUM);

  unsigned long secs = millis() / 1000;
  char upBuf[24];
  snprintf(upBuf, sizeof(upBuf), "%luh %lum %lus", secs / 3600, (secs % 3600) / 60, secs % 60);
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_PANEL);
  drawString(tft, "Uptime", 8, y + 28, TL_DATUM);
  tft.setTextColor(COLOR_TEXT, COLOR_PANEL);
  drawString(tft, upBuf, 50, y + 28, TL_DATUM);
  y += 46;

  // Navigation hint
  tft.setTextColor(COLOR_ACCENT, COLOR_BG);
  drawString(tft, "Tap center to toggle UI mode", MARGIN + 2, y + 2, TL_DATUM);

  drawScreenIndicator(tft, 6, 7);
}