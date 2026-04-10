#pragma once
#include <TFT_eSPI.h>

// ── Color Palette ─────────────────────────────────────────────────────────
// Deep dark theme with vibrant accents (same as original)
#define COLOR_BG         0x0841  // Very dark blue-gray (#0c1117)
#define COLOR_PANEL      0x1082  // Dark panel (#10192e)
#define COLOR_PANEL_ALT  0x18E3  // Slightly lighter panel
#define COLOR_CARD       0x2124  // Card background
#define COLOR_BORDER     0x2965  // Subtle border
#define COLOR_TEXT        0xFFFF  // White
#define COLOR_TEXT_DIM    0x9CF3  // Light gray
#define COLOR_TEXT_MUTED  0x6B6D  // Muted gray
#define COLOR_ACCENT     0x2E7F  // Bright cyan (#22d3ee)
#define COLOR_ACCENT_DIM 0x1C1F  // Darker cyan
#define COLOR_GREEN      0x2EC8  // Bright green (#22c55e)
#define COLOR_GREEN_DIM  0x1CA4  // Dark green
#define COLOR_RED        0xF8A6  // Bright red (#ef4444)
#define COLOR_RED_DIM    0xA000  // Dark red
#define COLOR_ORANGE     0xFC60  // Orange (#f97316)
#define COLOR_YELLOW     0xFFE0  // Yellow (#eab308)
#define COLOR_BLUE       0x3C1F  // Blue (#3b82f6)
#define COLOR_PURPLE     0xA01F  // Purple (#a855f7)
#define COLOR_PINK       0xF81F  // Pink accent

// Threat colors
#define COLOR_THREAT_CRIT  0xF800
#define COLOR_THREAT_HIGH  0xFC00
#define COLOR_THREAT_MED   0xFFE0
#define COLOR_THREAT_LOW   0x07E0

// ── Layout Constants ──────────────────────────────────────────────────────
#define SCREEN_W  320
#define SCREEN_H  240
#define HEADER_H  24
#define FOOTER_H  14
#define CONTENT_Y (HEADER_H + 1)
#define CONTENT_H (SCREEN_H - HEADER_H - FOOTER_H - NAV_BAR_H - 2)
#define MARGIN    4
#define LINE_H    16
#define SMALL_LINE_H 12
#define NAV_BAR_H 36   // Touch nav bar at bottom

// ── Text Helpers (TFT_eSPI) ──────────────────────────────────────────────

// TFT_eSPI datum constants (already defined in library)
#define TL_DATUM 0  // Top left
#define TC_DATUM 1  // Top centre
#define TR_DATUM 2  // Top right
#define ML_DATUM 3  // Middle left
#define MC_DATUM 4  // Middle centre
#define MR_DATUM 5  // Middle right
#define BL_DATUM 6  // Bottom left
#define BC_DATUM 7  // Bottom centre
#define BR_DATUM 8  // Bottom right

inline int textPixelWidth(TFT_eSPI& tft, const char* str) {
  return tft.textWidth(str);
}

inline void drawString(TFT_eSPI& tft, const char* str, int x, int y, int datum = TL_DATUM) {
  tft.setTextDatum(datum);
  tft.drawString(str, x, y);
}

inline void drawTruncated(TFT_eSPI& tft, const char* str, int x, int y, int maxW) {
  char buf[128];
  strlcpy(buf, str, sizeof(buf));
  while (textPixelWidth(tft, buf) > maxW && strlen(buf) > 3) {
    buf[strlen(buf) - 1] = '\0';
  }
  if (strlen(buf) < strlen(str) && strlen(buf) > 3) {
    buf[strlen(buf) - 1] = '.';
    buf[strlen(buf) - 2] = '.';
  }
  drawString(tft, buf, x, y, TL_DATUM);
}

// ── Drawing Primitives ───────────────────────────────────────────────────

inline void drawPanel(TFT_eSPI& tft, int x, int y, int w, int h, uint16_t bg = COLOR_PANEL, uint16_t border = COLOR_BORDER) {
  tft.fillRoundRect(x, y, w, h, 3, bg);
  tft.drawRoundRect(x, y, w, h, 3, border);
}

inline void drawHeader(TFT_eSPI& tft, const char* title, uint16_t accentColor = COLOR_ACCENT) {
  tft.fillRect(0, 0, SCREEN_W, HEADER_H, COLOR_PANEL);
  tft.drawFastHLine(0, HEADER_H - 2, SCREEN_W, accentColor);
  tft.drawFastHLine(0, HEADER_H - 1, SCREEN_W, COLOR_BORDER);

  tft.setTextSize(1);
  tft.setTextColor(accentColor, COLOR_PANEL);
  drawString(tft, title, MARGIN + 2, HEADER_H / 2, ML_DATUM);

  tft.fillCircle(MARGIN - 1, HEADER_H / 2, 2, accentColor);

  tft.setTextColor(COLOR_TEXT_MUTED, COLOR_PANEL);
  drawString(tft, "WM", SCREEN_W - MARGIN, HEADER_H / 2, MR_DATUM);
}

inline void drawStatusBar(TFT_eSPI& tft, const char* msg, uint16_t color) {
  tft.fillRect(0, SCREEN_H - FOOTER_H, SCREEN_W, FOOTER_H, COLOR_PANEL);
  tft.drawFastHLine(0, SCREEN_H - FOOTER_H, SCREEN_W, COLOR_BORDER);
  tft.setTextSize(1);
  tft.setTextColor(color, COLOR_PANEL);
  drawString(tft, msg, MARGIN, SCREEN_H - FOOTER_H / 2, ML_DATUM);
}

inline void drawScreenIndicator(TFT_eSPI& tft, int current, int total) {
  int gap = 12;
  int totalWidth = (total - 1) * gap + 10;
  int startX = (SCREEN_W - totalWidth) / 2;
  int y = SCREEN_H - FOOTER_H / 2;

  int x = startX;
  for (int i = 0; i < total; i++) {
    if (i == current) {
      tft.fillRoundRect(x - 4, y - 2, 10, 5, 2, COLOR_ACCENT);
      x += 10 + (gap - 4);
    } else {
      tft.fillCircle(x, y, 1, COLOR_TEXT_MUTED);
      x += gap;
    }
  }
}

inline void drawSplashScreen(TFT_eSPI& tft) {
  tft.fillScreen(0x0000);

  for (int r = 80; r > 10; r -= 8) {
    uint16_t c = (r > 50) ? COLOR_BORDER : (r > 30) ? COLOR_ACCENT_DIM : COLOR_ACCENT;
    tft.drawCircle(SCREEN_W / 2, SCREEN_H / 2 - 15, r, c);
    delay(30);
  }

  tft.drawFastHLine(SCREEN_W / 2 - 90, SCREEN_H / 2 - 15, 180, COLOR_BORDER);
  tft.drawFastVLine(SCREEN_W / 2, SCREEN_H / 2 - 15 - 90, 180, COLOR_BORDER);

  tft.fillCircle(SCREEN_W / 2, SCREEN_H / 2 - 15, 4, COLOR_ACCENT);
  tft.fillCircle(SCREEN_W / 2, SCREEN_H / 2 - 15, 2, COLOR_TEXT);

  tft.setTextSize(2);
  tft.setTextColor(COLOR_ACCENT);
  drawString(tft, "WORLDMONITOR", SCREEN_W / 2, SCREEN_H / 2 + 40, MC_DATUM);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT_DIM);
  drawString(tft, "CYD EDITION", SCREEN_W / 2, SCREEN_H / 2 + 60, MC_DATUM);

  tft.fillRoundRect(SCREEN_W / 2 - 18, SCREEN_H / 2 + 72, 36, 12, 3, COLOR_PANEL);
  tft.drawRoundRect(SCREEN_W / 2 - 18, SCREEN_H / 2 + 72, 36, 12, 3, COLOR_ACCENT_DIM);
  tft.setTextColor(COLOR_ACCENT);
  drawString(tft, "v2.0", SCREEN_W / 2, SCREEN_H / 2 + 78, MC_DATUM);
}

// ── Gauge ────────────────────────────────────────────────────────────────
inline void drawGauge(TFT_eSPI& tft, int cx, int cy, int radius, int value, uint16_t color) {
  for (float a = 3.14159; a >= 0; a -= 0.04) {
    int x = cx + (int)(cos(a) * radius);
    int y = cy - (int)(sin(a) * radius);
    tft.drawPixel(x, y, COLOR_BORDER);
    int x2 = cx + (int)(cos(a) * (radius - 1));
    int y2 = cy - (int)(sin(a) * (radius - 1));
    tft.drawPixel(x2, y2, COLOR_BORDER);
  }

  float endAngle = 3.14159 * (1.0 - value / 100.0);
  for (float a = 3.14159; a >= endAngle; a -= 0.03) {
    for (int t = 0; t < 3; t++) {
      int x = cx + (int)(cos(a) * (radius - t));
      int y = cy - (int)(sin(a) * (radius - t));
      tft.drawPixel(x, y, color);
    }
  }

  float needleAngle = 3.14159 * (1.0 - value / 100.0);
  int nx = cx + (int)(cos(needleAngle) * (radius - 6));
  int ny = cy - (int)(sin(needleAngle) * (radius - 6));
  tft.drawLine(cx, cy, nx, ny, COLOR_TEXT);
  tft.fillCircle(cx, cy, 2, COLOR_TEXT);
}

// ── Bar ──────────────────────────────────────────────────────────────────
inline void drawBar(TFT_eSPI& tft, int x, int y, int w, int h, float value, float maxVal, uint16_t color) {
  tft.fillRect(x, y, w, h, COLOR_PANEL);
  int fillW = (int)((value / maxVal) * w);
  if (fillW > 0) {
    tft.fillRect(x, y, fillW, h, color);
    tft.drawFastHLine(x, y, fillW, COLOR_TEXT);
  }
}

// ── Sparkline ────────────────────────────────────────────────────────────
inline void drawSparkline(TFT_eSPI& tft, int x, int y, int w, int h, float* data, int len) {
  if (len < 2) return;
  float minV = data[0], maxV = data[0];
  for (int i = 1; i < len; i++) {
    if (data[i] < minV) minV = data[i];
    if (data[i] > maxV) maxV = data[i];
  }
  float range = maxV - minV;
  if (range < 0.001f) range = 1.0f;
  float stepX = (float)w / (len - 1);

  bool up = data[len - 1] >= data[0];
  uint16_t lineColor = up ? COLOR_GREEN : COLOR_RED;
  uint16_t fillColor = up ? COLOR_GREEN_DIM : COLOR_RED_DIM;

  for (int i = 0; i < len; i++) {
    int px = x + (int)(i * stepX);
    int py = y + h - (int)(((data[i] - minV) / range) * h);
    tft.drawFastVLine(px, py, y + h - py, fillColor);
  }

  for (int i = 1; i < len; i++) {
    int x1 = x + (int)((i - 1) * stepX);
    int y1 = y + h - (int)(((data[i - 1] - minV) / range) * h);
    int x2 = x + (int)(i * stepX);
    int y2 = y + h - (int)(((data[i] - minV) / range) * h);
    tft.drawLine(x1, y1, x2, y2, lineColor);
  }
}

// ── Badge ────────────────────────────────────────────────────────────────
inline void drawBadge(TFT_eSPI& tft, int x, int y, const char* text, uint16_t bg, uint16_t fg = COLOR_TEXT) {
  int w = textPixelWidth(tft, text) + 6;
  tft.fillRoundRect(x, y, w, 10, 3, bg);
  tft.setTextColor(fg, bg);
  drawString(tft, text, x + 3, y + 1, TL_DATUM);
}

// ── Color Helpers ────────────────────────────────────────────────────────

inline uint16_t threatColor(int level) {
  switch (level) {
    case 4:  return COLOR_THREAT_CRIT;
    case 3:  return COLOR_THREAT_HIGH;
    case 2:  return COLOR_THREAT_MED;
    case 1:  return COLOR_THREAT_LOW;
    default: return COLOR_TEXT_MUTED;
  }
}

inline uint16_t riskColor(int score) {
  if (score >= 80) return COLOR_RED;
  if (score >= 65) return COLOR_ORANGE;
  if (score >= 45) return COLOR_YELLOW;
  if (score >= 25) return COLOR_GREEN;
  return COLOR_TEXT_MUTED;
}

inline uint16_t changeColor(float change) {
  if (change > 0.01f)  return COLOR_GREEN;
  if (change < -0.01f) return COLOR_RED;
  return COLOR_TEXT_DIM;
}

inline const char* trendArrow(const char* trend) {
  if (strcmp(trend, "UP") == 0 || strcmp(trend, "increasing") == 0)   return "^";
  if (strcmp(trend, "DOWN") == 0 || strcmp(trend, "decreasing") == 0) return "v";
  return "-";
}

inline uint16_t trendColor(const char* trend) {
  if (strcmp(trend, "UP") == 0 || strcmp(trend, "increasing") == 0)   return COLOR_RED;
  if (strcmp(trend, "DOWN") == 0 || strcmp(trend, "decreasing") == 0) return COLOR_GREEN;
  return COLOR_TEXT_MUTED;
}

inline uint16_t fgColor(int score) {
  if (score >= 75) return COLOR_GREEN;
  if (score >= 55) return 0x9FE0;
  if (score >= 45) return COLOR_YELLOW;
  if (score >= 25) return COLOR_ORANGE;
  return COLOR_RED;
}
