#pragma once
#include "config.h"

// ── Mock data for sources that need auth keys (ACLED, LLM, etc.) ─────────
// Markets, Fear/Greed, and Natural Events are fetched LIVE from public APIs.
// Risk scores, news, and forecasts require auth-gated sources — mock for now.

void loadMockRiskNewsForecasts(RiskScores& risk, NewsDigest& news, Forecasts& fc) {

  Serial.println(F("Loading mock risk/news/forecasts..."));

  // ── Risk Scores ─────────────────────────────────────────────────────
  risk.count = 8;

  auto setRisk = [](RiskItem& r, const char* region, int score, int dyn,
                    const char* trend, int newsAct, int milAct) {
    strlcpy(r.region, region, sizeof(r.region));
    r.combinedScore = score;
    r.dynamicScore = dyn;
    strlcpy(r.trend, trend, sizeof(r.trend));
    r.newsActivity = newsAct;
    r.militaryActivity = milAct;
  };

  setRisk(risk.items[0], "Taiwan Strait",     82, 85, "UP",     92, 78);
  setRisk(risk.items[1], "Ukraine-Russia",     78, 76, "STABLE", 88, 85);
  setRisk(risk.items[2], "Middle East",        72, 74, "UP",     85, 68);
  setRisk(risk.items[3], "Korean Peninsula",   58, 60, "UP",     62, 55);
  setRisk(risk.items[4], "South China Sea",    54, 52, "STABLE", 58, 48);
  setRisk(risk.items[5], "Iran-Israel",        65, 68, "UP",     72, 60);
  setRisk(risk.items[6], "Sahel Region",       42, 40, "DOWN",   45, 38);
  setRisk(risk.items[7], "India-Pakistan",     35, 34, "STABLE", 32, 28);

  // ── News ────────────────────────────────────────────────────────────
  news.count = 10;
  strlcpy(news.worldBrief,
    "Tensions rise in the Taiwan Strait as military exercises expand. "
    "Oil prices volatile amid Middle East uncertainty. Fed signals patience on rates.",
    sizeof(news.worldBrief));

  auto setNews = [](NewsItem& n, const char* title, const char* src,
                    const char* region, const char* cat, int threat,
                    bool alert, float imp) {
    strlcpy(n.title, title, sizeof(n.title));
    strlcpy(n.source, src, sizeof(n.source));
    strlcpy(n.region, region, sizeof(n.region));
    strlcpy(n.category, cat, sizeof(n.category));
    n.threatLevel = threat;
    n.isAlert = alert;
    n.importanceScore = imp;
    n.publishedAt = 0;
  };

  setNews(news.items[0], "China launches large-scale naval exercises near Taiwan",
          "Reuters", "TW", "Military", 4, true, 0.95f);
  setNews(news.items[1], "Fed holds rates steady, signals data-dependent approach",
          "Bloomberg", "US", "Economy", 1, false, 0.82f);
  setNews(news.items[2], "Israel-Iran tensions escalate after drone incident",
          "AP News", "IL", "Conflict", 3, true, 0.91f);
  setNews(news.items[3], "Oil surges 3% on Middle East supply concerns",
          "CNBC", "SA", "Markets", 2, false, 0.78f);
  setNews(news.items[4], "Ukraine counter-offensive gains ground in Zaporizhzhia",
          "BBC", "UA", "Conflict", 3, false, 0.88f);
  setNews(news.items[5], "Japan earthquake M6.2 strikes off Hokkaido coast",
          "NHK", "JP", "Natural", 2, false, 0.72f);
  setNews(news.items[6], "EU imposes new sanctions on Russian energy sector",
          "FT", "EU", "Policy", 2, false, 0.68f);
  setNews(news.items[7], "North Korea tests ballistic missile over Sea of Japan",
          "Yonhap", "KP", "Military", 3, true, 0.93f);
  setNews(news.items[8], "Gold hits record high amid geopolitical uncertainty",
          "WSJ", "US", "Markets", 1, false, 0.65f);
  setNews(news.items[9], "AI regulation framework proposed by G7 nations",
          "The Guardian", "GB", "Technology", 1, false, 0.55f);

  // ── Forecasts ───────────────────────────────────────────────────────
  fc.count = 6;

  auto setFc = [](Forecast& f, const char* title, const char* domain,
                  const char* region, float prob, float prior, float conf,
                  const char* horizon, const char* trend) {
    strlcpy(f.title, title, sizeof(f.title));
    strlcpy(f.domain, domain, sizeof(f.domain));
    strlcpy(f.region, region, sizeof(f.region));
    f.scenario[0] = '\0';
    f.probability = prob;
    f.priorProbability = prior;
    f.confidence = conf;
    strlcpy(f.timeHorizon, horizon, sizeof(f.timeHorizon));
    strlcpy(f.trend, trend, sizeof(f.trend));
  };

  setFc(fc.items[0], "China-Taiwan military confrontation (90 days)",
        "military", "Taiwan Strait", 0.35f, 0.28f, 0.55f, "90d", "increasing");
  setFc(fc.items[1], "Fed rate cut by September",
        "economy", "US", 0.62f, 0.65f, 0.72f, "6mo", "decreasing");
  setFc(fc.items[2], "Oil above $90/bbl sustained",
        "markets", "Global", 0.41f, 0.38f, 0.60f, "3mo", "increasing");
  setFc(fc.items[3], "Russia-Ukraine ceasefire agreement",
        "conflict", "Europe", 0.12f, 0.14f, 0.45f, "12mo", "stable");
  setFc(fc.items[4], "Major cyberattack on Western infrastructure",
        "cyber", "NATO", 0.28f, 0.22f, 0.50f, "6mo", "increasing");
  setFc(fc.items[5], "S&P 500 correction (>10% drawdown)",
        "markets", "US", 0.33f, 0.30f, 0.58f, "6mo", "increasing");

  Serial.println(F("Mock risk/news/forecasts loaded"));
}