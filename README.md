# 🌍 WorldMonitor CYD Edition

**A touch‑screen global dashboard on a $15 ESP32 display.**

This is a complete port of the **WorldMonitor Desktop Companion** to the ESP32 Cheap Yellow Display (CYD) — a 2.8" touchscreen that shows live markets, earthquakes, natural events, and risk scores. No joystick, no SD card, no extra hardware. Just the CYD and a USB cable.

### ✨ What it does

- **Fetches live data** from free public APIs (no keys required):
  - 📈 Yahoo Finance – stock indices, VIX, yields
  - 🌍 USGS – earthquakes
  - 🔥 NASA EONET – volcanoes, wildfires, storms
- **Displays risk scores, news, and forecasts** (mock data, same as original)
- **Two UI modes** – Simple (4 screens) or Classic (7 screens)
- **Touch‑screen navigation** – on‑screen buttons + tap‑center actions
- **Auto‑advance kiosk mode** – hides UI, cycles data screens when untouched
- **WiFi + config** – stores settings in ESP32 flash, optional SD‑card config
- **Dark theme** – high‑contrast, easy‑on‑eyes colors

### 🧱 Hardware

- ESP32 CYD (Cheap Yellow Display) 2.8" 320×240 ILI9341 + XPT2046 touch
- USB‑C cable for power/programming
- WiFi network

### ⚙️ Software

- Arduino IDE or PlatformIO
- TFT_eSPI library (configured for CYD)
- ArduinoJson, Preferences (built‑in)

---

## 🚀 Quick Start

1. **Install Arduino IDE** or PlatformIO.
2. **Add ESP32 board support** via Boards Manager.
3. **Install libraries**: TFT_eSPI, ArduinoJson.
4. **Configure TFT_eSPI** for CYD (edit `User_Setup.h`):
   ```cpp
   #define ILI9341_DRIVER
   #define TFT_WIDTH  320
   #define TFT_HEIGHT 240
   #define TFT_CS   15
   #define TFT_DC    2
   #define TFT_RST  -1
   #define TFT_BL   21
   #define TOUCH_CS 33
   ```
5. **Open `WorldMonitor_CYD.ino`**, select **ESP32 Dev Module**, upload.
6. **Set WiFi** via Serial Monitor or edit `config.h`.

## 🛠️ Building with PlatformIO

If you prefer PlatformIO (VS Code):

1. Install [PlatformIO](https://platformio.org/) extension in VS Code.
2. Open the project folder (`WorldMonitor_CYD`).
3. The `platformio.ini` file already configures the ESP32 board and required libraries.
4. Connect your CYD via USB.
5. Click **Upload** in the PlatformIO toolbar.

PlatformIO will automatically download TFT_eSPI and ArduinoJson. You still need to edit the TFT_eSPI `User_Setup.h` as described above.

## ⚙️ Configuration

### WiFi
- Default credentials: `YourWiFi` / `YourPassword`.
- Change in `config.h` or via SD card.
- Credentials saved to ESP32 NVS flash.

### SD Card (Optional)
- Insert SD card with `wmconfig.json`:
   ```json
   {
     "wifi_ssid": "YourWiFi",
     "wifi_password": "YourPassword",
     "ui_mode": "simple",
     "refresh_seconds": 60,
     "auto_advance_seconds": 30
   }
   ```
- Priority: **SD → NVS → defaults**.
- Config changes saved to both SD (if present) and NVS.

## 🎮 Navigation

- **Bottom bar buttons**: `<` (prev), `UP` (scroll up), `REFR` (refresh/toggle UI), `DOWN` (scroll down), `>` (next).
- **Tap center of content area** – refresh data (or toggle UI mode on Settings screen).
- **Screen dots** show current screen / total.

### UI Modes
- **Simple** (default) – 4 screens: Summary, Markets, Alerts, Settings.
- **Classic** – 7 screens: Dashboard, Markets, Risk, News, Events, Forecasts, Settings.
- Toggle by tapping **center button** on **Settings screen**.

## 🔄 Auto‑advance (Kiosk Mode)

If no touch for `auto_advance_seconds` (default 30):
- **Navigation bar hidden** – full‑screen data view.
- **Settings screen skipped** – only data screens rotate.
- **UI returns** on any touch.

Adjust in `config.h`:
```cpp
cfg.autoAdvanceSeconds = 30; // 0 = disable
```

## 🐛 Touch Debug

Uncomment `#define TOUCH_DEBUG` in `WorldMonitor_CYD.ino` to print raw touch coordinates to Serial Monitor.

## 📁 Files

| File | Purpose |
|------|---------|
| `WorldMonitor_CYD.ino` | Main sketch (touch, WiFi, config, refresh) |
| `config.h` | Data structures, NVS/SD config functions |
| `ui.h` | TFT_eSPI drawing primitives, colors |
| `screens.h` | All screen‑drawing functions (Simple + Classic) |
| `api.h` | HTTP clients for Yahoo Finance, USGS, NASA |
| `mockdata.h` | Mock risk/news/forecasts data |

## 🔧 Troubleshooting

| Issue | Likely fix |
|-------|------------|
| Touch not working | Check `TOUCH_CS = 33` in TFT_eSPI config, calibrate `calData` |
| Colors inverted | CYD needs `tft.invertDisplay(true)` (already set) |
| WiFi fails | Check power, SSID/password in `config.h` |
| Live data stale | WiFi must be connected, min 60‑second refresh |

## 🙏 Credits

- Original **WorldMonitor Companion** by [Dag](https://github.com/dagnazty)
- **CYD adaptation** with touch navigation
- **TFT_eSPI** library by Bodmer
- **Public APIs**: Yahoo Finance, USGS, NASA EONET

## 📄 License

Open source, based on original WorldMonitor Desktop Companion.

## 🚀 Publishing to GitHub

To upload this project as a standalone repository:

```bash
# Clone this folder to a new location (optional)
cp -r WorldMonitor_CYD ~/your-new-repo
cd ~/your-new-repo

# Initialize a new Git repository
git init
git add .
git commit -m "Initial commit: WorldMonitor CYD Edition"

# Create a new repository on GitHub (no README, no .gitignore)
# Then add the remote and push
git remote add origin https://github.com/your-username/your-repo.git
git branch -M main
git push -u origin main
```

All necessary GitHub files (`.gitignore`, `.gitattributes`, `LICENSE`) are already included.