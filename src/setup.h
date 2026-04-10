// TFT_eSPI configuration for ESP32 CYD (Cheap Yellow Display)
// Place this file in the same folder as your sketch (src/).
// TFT_eSPI will automatically include it.

#ifndef SETUP_H
#define SETUP_H

#ifndef USER_SETUP_LOADED
#define USER_SETUP_LOADED
#endif

#ifndef ILI9341_DRIVER
#define ILI9341_DRIVER
#endif
#ifndef TFT_WIDTH
#define TFT_WIDTH  320
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 240
#endif

// CYD pins (ESP32)
#ifndef TFT_CS
#define TFT_CS   15
#endif
#ifndef TFT_DC
#define TFT_DC    2
#endif
#ifndef TFT_RST
#define TFT_RST  -1   // Not connected on CYD
#endif
#ifndef TFT_BL
#define TFT_BL   21   // Backlight control
#endif

#ifndef TOUCH_CS
#define TOUCH_CS 33   // XPT2046 touch CS
#endif

// SPI frequencies
#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY          40000000
#endif
#ifndef SPI_READ_FREQUENCY
#define SPI_READ_FREQUENCY     20000000
#endif
#ifndef SPI_TOUCH_FREQUENCY
#define SPI_TOUCH_FREQUENCY    2500000
#endif

// Optional: enable fonts
#ifndef LOAD_GLCD
#define LOAD_GLCD
#endif
#ifndef LOAD_FONT2
#define LOAD_FONT2
#endif
#ifndef LOAD_FONT4
#define LOAD_FONT4
#endif
#ifndef LOAD_FONT6
#define LOAD_FONT6
#endif
#ifndef LOAD_FONT7
#define LOAD_FONT7
#endif
#ifndef LOAD_FONT8
#define LOAD_FONT8
#endif
#ifndef LOAD_GFXFF
#define LOAD_GFXFF
#endif
#ifndef SMOOTH_FONT
#define SMOOTH_FONT
#endif

#endif // SETUP_H