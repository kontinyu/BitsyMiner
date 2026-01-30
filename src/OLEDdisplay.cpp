/*
 * BitsyMiner Open Source - OLED Display Implementation
 * Copyright (c) 2025 Justin Williams
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include "defines_n_types.h"

#ifdef USE_OLED
#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#ifdef USE_OLED_SPI
  #include <SPI.h>
#else
  #include <Wire.h>
#endif

#include "monitor.h"
#include "utils.h"
#include "MyWiFi.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

extern SetupData settings;
extern MonitorData monitorData;

uint8_t currentScreen = SCREEN_MINING;
uint8_t currentScreenOrientation = 0;

// Screen cycling variables
unsigned long lastScreenChangeTime = 0;
const unsigned long SCREEN_CYCLE_INTERVAL = 300000; // 15 seconds in milliseconds

#ifdef USE_OLED_SPI
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_DC, OLED_RST);
#else
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
#endif

#define OLED_WHITE 1
#define OLED_BLACK 0
#define NAME "QMiner"

///////////////////////////////////////////////////////////////////////////////////////////
// API Data Variables
///////////////////////////////////////////////////////////////////////////////////////////
String apiBtcPrice = "--";
String apiBlockHeight = "--";
String apiGlobalHashrate = "--";
String apiDifficulty = "--";
double apiDifficultyRaw = 0.0;
String apiMinerMotivation = "";
unsigned long lastApiCall = 0;
bool apiDataFetched = false;
uint8_t lastLoadedScreen = 0; // Track which screen was last loaded for API fetching

///////////////////////////////////////////////////////////////////////////////////////////
// Scrolling Text Variables for Screen 4
///////////////////////////////////////////////////////////////////////////////////////////
unsigned long lastScrollTime = 0;
int scrollOffset = 0;
const int scrollSpeed = 50; // milliseconds between scroll updates
unsigned long screen4LoadTime = 0; // Track when screen 4 was loaded
const unsigned long SCROLL_START_DELAY = 3000; // 3 second delay before scrolling begins
bool scrollDelayActive = false; // Flag to track if we're in delay period

///////////////////////////////////////////////////////////////////////////////////////////
// Fetch API data from kontinyu server
///////////////////////////////////////////////////////////////////////////////////////////
void fetchKontinyuAPI() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  // Only fetch if we don't have fresh data (cache for 30 minutes)
  if (apiDataFetched && (millis() - lastApiCall < 30 * 60 * 1000)) return;
  
  HTTPClient http;
  http.setTimeout(5000);
  http.begin("http://miner.kontinyu.com/api.php");
  
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      if (doc.containsKey("btcPrice")) {
        apiBtcPrice = doc["btcPrice"].as<String>();
      }
      if (doc.containsKey("blockHeight")) {
        apiBlockHeight = doc["blockHeight"].as<String>();
      }
      if (doc.containsKey("globalHashrate")) {
        apiGlobalHashrate = doc["globalHashrate"].as<String>();
      }
      if (doc.containsKey("difficulty")) {
        apiDifficulty = doc["difficulty"].as<String>();
      }
      if (doc.containsKey("difficultyRaw")) {
        apiDifficultyRaw = doc["difficultyRaw"].as<double>();
      }
      if (doc.containsKey("minermotivation")) {
        apiMinerMotivation = doc["minermotivation"].as<String>();
      }
      apiDataFetched = true;
      lastApiCall = millis(); // Update timestamp on success
    }
    doc.clear();
  }
  // If fetch fails, don't update lastApiCall - allow retry on next screen load
  http.end();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Format luck odds with appropriate scale (Sx, Qi, Q, T, B, M, etc.)
///////////////////////////////////////////////////////////////////////////////////////////
String formatLuckOdds(double odds) {
  if (odds >= 1e21) {
    double sextillions = odds / 1e21;
    return String((long)sextillions) + "Sx";
  } else if (odds >= 1e18) {
    double quintillions = odds / 1e18;
    return String((long)quintillions) + "Qi";
  } else if (odds >= 1e15) {
    double quadrillions = odds / 1e15;
    return String((long)quadrillions) + "Q";
  } else if (odds >= 1e12) {
    double trillions = odds / 1e12;
    return String((long)trillions) + "T";
  } else if (odds >= 1e9) {
    double billions = odds / 1e9;
    return String((long)billions) + "B";
  } else if (odds >= 1e6) {
    double millions = odds / 1e6;
    return String((long)millions) + "M";
  } else {
    return String((long)odds);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Calculate luck based on API difficulty and best difficulty found
///////////////////////////////////////////////////////////////////////////////////////////
String calculateLuck() {
  String luckNumber = "--";
  if (apiDataFetched && apiDifficultyRaw > 0) {
    // Use bestDifficultyStr from monitor data
    String bestDiffStr = monitorData.bestDifficultyStr;
    double bestDiff = bestDiffStr.toDouble();
    if (bestDiff > 0) {
      double odds = apiDifficultyRaw / bestDiff;
      luckNumber = formatLuckOdds(odds);
    }
  }
  return luckNumber;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Auto-cycle through screens
///////////////////////////////////////////////////////////////////////////////////////////
void updateScreenCycle() {
  // Only cycle if NOT on access point screen
  if (currentScreen == SCREEN_ACCESS_POINT) {
    lastScreenChangeTime = millis(); // Reset timer when on AP screen
    return;
  }
  
  unsigned long currentTime = millis();
  if (currentTime - lastScreenChangeTime >= SCREEN_CYCLE_INTERVAL) {
    // Time to cycle to next screen
    uint8_t nextScreen;
    
    // Cycle through: SCREEN_MINING(1) -> 3 -> 4 -> 5 -> 6 -> SCREEN_MINING(1)
    switch (currentScreen) {
      case SCREEN_MINING:
        nextScreen = 3;
        break;
      case 3:
        nextScreen = 4;
        break;
      case 4:
        nextScreen = 5;
        break;
      case 5:
        nextScreen = 6;
        break;
      case 6:
        nextScreen = SCREEN_MINING;
        break;
      default:
        nextScreen = SCREEN_MINING;
        break;
    }
    
    setCurrentScreen(nextScreen);
    lastScreenChangeTime = currentTime;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Initialize OLED display
///////////////////////////////////////////////////////////////////////////////////////////
void initializeDisplay(uint8_t rotation, uint8_t brightness) {
  currentScreenOrientation = rotation;

#ifdef USE_OLED_SPI
  // Initialize hardware SPI with defined pins
  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);
#else
  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);
#endif
  
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "BitsyMiner");
  u8g2.drawStr(0, 25, "Initializing...");
  u8g2.sendBuffer();
}

///////////////////////////////////////////////////////////////////////////////////////////
// OLED Mining Screen
///////////////////////////////////////////////////////////////////////////////////////////
void refreshOLEDMiningScreen() {

  u8g2.clearBuffer();
  
  // "Qminer" title
  u8g2.setFont(u8g2_font_sirclive_tr);
  u8g2.drawStr(2, 12, NAME);
  
  u8g2.setFont(u8g2_font_profont12_tr);
  u8g2.drawStr(102, 13, "KH/s");
  
  // Line
  u8g2.drawLine(0, 16, 127, 16);
  
  // Hashrate
  u8g2.setFont(u8g2_font_7_Seg_41x21_mn); // Classic digital display look
  u8g2.drawStr(2, 20, monitorData.hashesPerSecondStr);
  
/*
  u8g2.setCursor(0, 58);
  u8g2.print("Blks: ");
  u8g2.print(monitorData.validBlocksFoundStr);
  u8g2.print(" ");
  if (monitorData.isMining) {
    u8g2.print("MINING");
  } else {
    u8g2.print("IDLE");
  }
  */
  u8g2.sendBuffer();

}

///////////////////////////////////////////////////////////////////////////////////////////
// OLED Access Point Screen
///////////////////////////////////////////////////////////////////////////////////////////
void refreshOLEDAccessPointScreen() {

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Access Point Mode");
  
  u8g2.setCursor(0, 30);
  u8g2.print("IP: ");
  u8g2.print(MyWiFi::getIP().toString());
  
  u8g2.drawStr(0, 50, "Connect to WiFi");
  u8g2.drawStr(0, 60, "to configure");
  
  u8g2.sendBuffer();

}

///////////////////////////////////////////////////////////////////////////////////////////
// OLED Alternate Screen 2
///////////////////////////////////////////////////////////////////////////////////////////
void refreshOLEDScreen2() {
  u8g2.clearBuffer();

  // "Qminer" title
  u8g2.setFont(u8g2_font_sirclive_tr);
  u8g2.drawStr(2, 12, NAME);
  
  u8g2.setFont(u8g2_font_profont12_tr);
  u8g2.drawStr(88, 13, monitorData.hashesPerSecondStr);
  
  // Line
  u8g2.drawLine(0, 16, 127, 16);

  u8g2.setFont(u8g2_font_7_Seg_41x21_mn);
  
  char timeStr[6];
  Date d;
  dateFromEpoch(&d, monitorData.currentTime + settings.utcOffset);
  int hour = d.hour;
  if (!settings.clock24) {
    hour = (hour % 12) ? (hour % 12) : 12;
  }
  sprintf(timeStr, "%02d:%02d", hour, d.minute);
  u8g2.drawStr(10, 20, timeStr);
  
  
  u8g2.sendBuffer();
}

///////////////////////////////////////////////////////////////////////////////////////////
// OLED Alternate Screen 3 - Luck Calculator
///////////////////////////////////////////////////////////////////////////////////////////
void refreshOLEDScreen3() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_sirclive_tr);
  u8g2.drawStr(0, 12, NAME);
  
  u8g2.setFont(u8g2_font_profont12_tr);
  u8g2.drawStr(88, 13, monitorData.hashesPerSecondStr);
  
  // Line
  u8g2.drawLine(0, 16, 127, 16);
  
  // Calculate luck
  String luck = calculateLuck();
  
  u8g2.setFont(u8g2_font_helvB12_tf);
  
  // Line 1: "1 in" - centered
  int line1Width = u8g2.getStrWidth("1 in");
  int line1X = (128 - line1Width) / 2;
  u8g2.drawStr(line1X, 35, "1 in");
   u8g2.setFont(u8g2_font_helvB18_tf);
  // Line 2: luck number - centered
  int line2Width = u8g2.getStrWidth(luck.c_str());
  int line2X = (128 - line2Width) / 2;
  u8g2.drawStr(line2X, 58, luck.c_str());
  u8g2.sendBuffer();
}

///////////////////////////////////////////////////////////////////////////////////////////
// OLED Alternate Screen 4 - Scrolling Miner Motivation
///////////////////////////////////////////////////////////////////////////////////////////
void refreshOLEDScreen4() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_sirclive_tr);
  u8g2.drawStr(2, 12, NAME);
  
  u8g2.setFont(u8g2_font_profont12_tr);
  u8g2.drawStr(88, 13, monitorData.hashesPerSecondStr);
  
  // Line
  u8g2.drawLine(0, 16, 127, 16);
  
  // Set clipping to blue region only (hide text that scrolls into yellow region)
  u8g2.setClipWindow(0, 17, 127, 63);
  
  // Display miner motivation quote with vertical scrolling
  u8g2.setFont(u8g2_font_6x10_tf);
  
  // Get motivation quote
  String quote = apiMinerMotivation;
  if (quote.length() == 0) {
    quote = "Keep mining!";
  }
  
  // Remove all newlines - we'll do our own word wrapping
  quote.replace("\\n", " ");
  quote.replace("\n", " ");
  quote.trim();
  
  int lineHeight = 12;
  int blueRegionTop = 26; // Start of blue region
  int blueRegionBottom = 64; // End of display
  int blueRegionHeight = blueRegionBottom - blueRegionTop; // 38 pixels
  int maxWidth = 118; // Max width for text (128 - margins)
  
  // Build wrapped lines array with word wrapping by pixel width
  String wrappedLines[50]; // Max 50 lines
  int lineCount = 0;
  
  // Word wrap the entire quote
  String currentLine = "";
  int wordStart = 0;
  
  for (int i = 0; i <= quote.length(); i++) {
    if (i == quote.length() || quote.charAt(i) == ' ') {
      String word = quote.substring(wordStart, i);
      word.trim();
      
      if (word.length() == 0) {
        wordStart = i + 1;
        continue;
      }
      
      String testLine = currentLine.length() > 0 ? currentLine + " " + word : word;
      
      if (u8g2.getStrWidth(testLine.c_str()) <= maxWidth) {
        currentLine = testLine;
      } else {
        // Current line is full, save it and start new line with this word
        if (currentLine.length() > 0) {
          wrappedLines[lineCount++] = currentLine;
        }
        currentLine = word;
      }
      
      wordStart = i + 1;
      
      if (lineCount >= 50) break;
    }
  }
  
  // Add remaining line
  if (currentLine.length() > 0 && lineCount < 50) {
    wrappedLines[lineCount++] = currentLine;
  }
  
  // Calculate total content height
  int totalContentHeight = lineCount * lineHeight;
  
  // Only enable scrolling if we have 3+ lines
  bool shouldScroll = (lineCount > 3);
  
  // Track screen load time on first load
  if (screen4LoadTime == 0) {
    screen4LoadTime = millis();
    scrollOffset = 0; // Ensure we start at top
    lastScrollTime = millis();
    scrollDelayActive = true; // Start with delay active
  }
  
  // Check if delay period has ended
  if (scrollDelayActive && (millis() - screen4LoadTime >= SCROLL_START_DELAY)) {
    scrollDelayActive = false; // Delay is over, allow scrolling
  }
  
  // Update scroll offset (smooth pixel-by-pixel scrolling) - only if scrolling and delay has passed
  if (shouldScroll && !scrollDelayActive && (millis() - lastScrollTime > scrollSpeed)) {
    int maxScrollOffset = totalContentHeight - blueRegionHeight;

    if (scrollOffset < maxScrollOffset) {
      scrollOffset+=6; // Scroll 6 pixels per update
      lastScrollTime = millis();
    } else {
      delay(2000); // Wait for 2 seconds
      scrollOffset = 0; // Restart from top
      screen4LoadTime = millis(); // Reset load time to restart delay
      scrollDelayActive = true; // Activate delay again
      lastScrollTime = millis();
    }
  } else if (!shouldScroll) {
    // No scrolling - reset offset to show content from top
    scrollOffset = 0;
  }
  
  // Draw wrapped lines with scroll offset
  for (int i = 0; i < lineCount; i++) {
    int yPos = blueRegionTop + (i * lineHeight) - scrollOffset;
    
    // Draw if in visible range (clipping handles the rest)
    // Use >= instead of > to show first line at the boundary
    if (yPos >= blueRegionTop - lineHeight && yPos <= blueRegionBottom && wrappedLines[i].length() > 0) {
      u8g2.drawStr(5, yPos, wrappedLines[i].c_str());
    }
  }
  
  // Remove clipping
  u8g2.setMaxClipWindow();
  
  u8g2.sendBuffer();
}

///////////////////////////////////////////////////////////////////////////////////////////
// OLED Alternate Screen 5
///////////////////////////////////////////////////////////////////////////////////////////
void refreshOLEDScreen5() {
  u8g2.clearBuffer();
  
  // Header
  u8g2.setFont(u8g2_font_sirclive_tr);
  u8g2.drawStr(2, 12, NAME);
  
  // small status on right of header
  u8g2.setFont(u8g2_font_profont12_tr);
  u8g2.drawStr(85, 13, monitorData.hashesPerSecondStr);

  // Separator line
  u8g2.drawLine(0, 16, 127, 16);

  // Large centered blocks found number
  String blocksStr = String(monitorData.validBlocksFoundStr);
  u8g2.setFont(u8g2_font_freedoomr25_mn);
  u8g2.drawStr(20, 47, blocksStr.c_str());

  // Small caption below
  String caption = "blocks found";
  u8g2.setFont(u8g2_font_6x10_tf);
  int capW = u8g2.getStrWidth(caption.c_str());
  u8g2.drawStr(30, 55, caption.c_str());

  u8g2.sendBuffer();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Refresh the current screen
///////////////////////////////////////////////////////////////////////////////////////////
void refreshDisplay() {
  switch (currentScreen) {
    case SCREEN_MINING:
      refreshOLEDMiningScreen();
      break;
    case SCREEN_ACCESS_POINT:
      refreshOLEDAccessPointScreen();
      break;
    case 3:
      refreshOLEDScreen2();
      break;
    case 4:
      refreshOLEDScreen3();
      break;
    case 5:
      refreshOLEDScreen4();
      break;
    case 6:
      refreshOLEDScreen5();
      break;
    default:
      refreshOLEDMiningScreen();
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Redraw the current screen
///////////////////////////////////////////////////////////////////////////////////////////
void redraw() {
  u8g2.clearBuffer();
  refreshDisplay();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Set the current screen
///////////////////////////////////////////////////////////////////////////////////////////
void setCurrentScreen(uint8_t screen) {
  // Check if this is a new screen load (not just a refresh)
  if (screen != lastLoadedScreen) {
    lastLoadedScreen = screen;
    
    // Fetch API data when loading screens 3, 4, or 5
    if (screen == 3 || screen == 4 || screen == 5) {
      fetchKontinyuAPI();
    }
    
    // Reset scroll state when loading screen 5 (which is screen 4 content - scrolling motivation)
    if (screen == 5) {
      screen4LoadTime = 0; // Reset load time to trigger delay on next refresh
      scrollOffset = 0; // Reset scroll position
      scrollDelayActive = false; // Reset delay flag
    }
  }
  
  currentScreen = screen;
  lastScreenChangeTime = millis(); // Reset cycle timer when screen is manually set
  redraw();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Set rotation
///////////////////////////////////////////////////////////////////////////////////////////
void setRotation(uint8_t rotation) {
  currentScreenOrientation = rotation;

  // U8g2 rotation is typically set in the constructor (U8G2_R0, etc.)
  //u8g2.setDisplayRotation(rotation);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Set brightness (OLED has limited brightness control)
///////////////////////////////////////////////////////////////////////////////////////////
void setBrightness(unsigned long brightness) {
  // OLED displays don't have brightness control in SSD1306
  // Some variants support contrast adjustment via dim()
  //display.dim(brightness < 128);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Check if screen is touched (OLED has no touch)
///////////////////////////////////////////////////////////////////////////////////////////
bool screenTouched() {
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Handle screen touch (OLED has no touch, but keep for compatibility)
///////////////////////////////////////////////////////////////////////////////////////////
void handleScreenTouch() {
  // No touchscreen on OLED
}

#endif // USE_OLED
