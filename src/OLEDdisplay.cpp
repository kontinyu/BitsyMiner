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

#ifdef USE_OLED_SPI
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_DC, OLED_RST);
#else
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);
#endif

#define OLED_WHITE 1
#define OLED_BLACK 0

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
  u8g2.setFont(u8g2_font_helvB08_tr);
  u8g2.drawStr(0, 12, "Qminer");
  
  // "KH/s" label
  u8g2.drawStr(92, 12, "KH/s");
  
  // Line
  u8g2.drawLine(0, 16, 127, 16);
  
  // Hashrate
  u8g2.setFont(u8g2_font_helvB14_tr);
  u8g2.setCursor(5, 36);
  u8g2.print(monitorData.hashesPerSecondStr);
  
  // Stats
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(0, 58);
  u8g2.print("Blks: ");
  u8g2.print(monitorData.validBlocksFoundStr);
  u8g2.print(" ");
  if (monitorData.isMining) {
    u8g2.print("MINING");
  } else {
    u8g2.print("IDLE");
  }
  
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
  u8g2.clearBuffer();
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
