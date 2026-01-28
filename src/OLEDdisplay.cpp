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
#ifdef USE_OLED_SPI
#include <SPI.h>
#include <Adafruit_SH110X.h>
#else
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

#ifdef USE_OLED_SPI
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);
#define OLED_WHITE SH110X_WHITE
#else
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define OLED_WHITE SSD1306_WHITE
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// Initialize OLED display
///////////////////////////////////////////////////////////////////////////////////////////
void initializeDisplay(uint8_t rotation, uint8_t brightness) {
  currentScreenOrientation = rotation;

#ifdef USE_OLED_SPI
  // Initialize Hardware SPI with defined pins (SCK, MISO, MOSI, SS)
  SPI.begin(OLED_CLK, -1, OLED_MOSI, -1);
  if(!display.begin(0, true)) { // 0 = Address (ignored for SPI), true = reset
    Serial.println(F("SH1106 allocation failed"));
    return;
  }
#else
  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
#endif

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(OLED_WHITE);
  display.setCursor(0, 0);
  display.println(F("BitsyMiner"));
  display.println(F("Initializing..."));
  display.display();
}

///////////////////////////////////////////////////////////////////////////////////////////
// OLED Mining Screen
///////////////////////////////////////////////////////////////////////////////////////////
void refreshOLEDMiningScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(OLED_WHITE);

  // Line 1: Hash rate
  display.setCursor(0, 0);
  display.print(F("H/s: "));
  display.println(monitorData.hashesPerSecondStr);

  // Line 2: Pool submissions
  display.setCursor(0, 10);
  display.print(F("Pool: "));
  display.println(monitorData.poolSubmissionsStr);

  // Line 3: Best difficulty
  display.setCursor(0, 20);
  display.print(F("Best: "));
  display.println(monitorData.bestDifficultyStr);

  // Line 4: Valid blocks
  display.setCursor(0, 30);
  display.print(F("Blks: "));
  display.println(monitorData.validBlocksFoundStr);

  // Line 5: WiFi/Pool status
  display.setCursor(0, 40);
  if (monitorData.wifiConnected) {
    display.print(F("WiFi:OK"));
  } else {
    display.print(F("WiFi:--"));
  }
  display.setCursor(64, 40);
  if (monitorData.poolConnected) {
    display.print(F("Pool:OK"));
  } else {
    display.print(F("Pool:--"));
  }

  // Line 6: Mining status
  display.setCursor(0, 50);
  if (monitorData.isMining) {
    display.print(F("Status: MINING"));
  } else {
    display.print(F("Status: IDLE"));
  }

  display.display();
}

///////////////////////////////////////////////////////////////////////////////////////////
// OLED Access Point Screen
///////////////////////////////////////////////////////////////////////////////////////////
void refreshOLEDAccessPointScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(OLED_WHITE);

  display.setCursor(0, 0);
  display.println(F("Access Point Mode"));
  display.println();
  display.print(F("IP: "));
  display.println(MyWiFi::getIP().toString());
  display.println();
  display.println(F("Connect to WiFi"));
  display.println(F("to configure"));

  display.display();
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
    default:
      refreshOLEDMiningScreen();
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Redraw the current screen
///////////////////////////////////////////////////////////////////////////////////////////
void redraw() {
  display.clearDisplay();
  refreshDisplay();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Set the current screen
///////////////////////////////////////////////////////////////////////////////////////////
void setCurrentScreen(uint8_t screen) {
  currentScreen = screen;
  redraw();
}

///////////////////////////////////////////////////////////////////////////////////////////
// Set rotation
///////////////////////////////////////////////////////////////////////////////////////////
void setRotation(uint8_t rotation) {
  currentScreenOrientation = rotation;
  display.setRotation(rotation);
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
