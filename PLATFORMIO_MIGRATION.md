# BitsyMiner - PlatformIO Migration

This project has been migrated from Arduino IDE to PlatformIO.

## Project Structure

```
BitsyMiner/
├── platformio.ini          # PlatformIO configuration file
├── src/                    # Source files (formerly BitsyMinerOpenSource/)
│   ├── main.cpp           # Main application file (formerly .ino)
│   ├── *.cpp              # All implementation files
│   └── *.h                # All header files
├── lib/                    # Custom libraries
│   └── TFT_eSPI_Config/   # TFT display configuration
├── include/                # Additional headers (if needed)
├── BitsyMinerOpenSource/  # Original Arduino IDE files (kept for reference)
├── TFT_eSPI/              # Original TFT config files (kept for reference)
├── assets/                # Asset files
└── binaries/              # Compiled binaries

```

## Build Environments

The project supports multiple hardware configurations:

### 1. ESP32-2432S028 (Default)
2.8" display with ILI9341 driver
```bash
pio run -e esp32_2432s028
```

### 2. ESP32-2432S028 with ST7789
2.8" display with ST7789 driver
```bash
pio run -e esp32_2432s028_st7789
```

### 3. ESP32-2432S024
2.4" display with ILI9341 driver
```bash
pio run -e esp32_2432s024
```

### 4. ESP32 Dev Headless
ESP32 without display support
```bash
pio run -e esp32_dev_headless
```

## Building and Uploading

### Build the project
```bash
pio run
```

### Upload to device
```bash
pio run --target upload
```

### Monitor serial output
```bash
pio device monitor
```

### Build, upload, and monitor (all in one)
```bash
pio run --target upload && pio device monitor
```

## Dependencies

The following libraries are automatically managed by PlatformIO (defined in platformio.ini):

- **ArduinoJson** (^7.0.0) - JSON parsing and generation
- **TFT_eSPI** (^2.5.43) - Display driver library
- **XPT2046_Touchscreen** (^1.4) - Touchscreen controller
- **PNGdec** (^1.0.2) - PNG image decoder

## Configuration

### TFT_eSPI Configuration
TFT_eSPI is configured via build flags in platformio.ini instead of User_Setup.h files. Each environment has its own display configuration.

### Hardware Defines
The hardware type is selected by the build environment:
- `-DESP32_2432S028` for 2.8" display
- `-DESP32_2432S024` for 2.4" display  
- `-DESP32_DEV_HEADLESS` for headless operation
- `-DST7789_LCD` for ST7789 LCD variant

## VS Code Integration

PlatformIO integrates seamlessly with VS Code:

1. Install the PlatformIO IDE extension
2. Open this folder in VS Code
3. Use the PlatformIO toolbar for build/upload/monitor operations
4. IntelliSense will work automatically with proper code completion

## Differences from Arduino IDE

1. **File naming**: `BitsyMinerOpenSource.ino` → `main.cpp`
2. **Project structure**: Organized in `src/`, `lib/`, `include/` folders
3. **Library management**: Automatic via `platformio.ini`
4. **Build configurations**: Multiple environments for different hardware
5. **TFT_eSPI setup**: Configured via build flags, not User_Setup.h

## Original Arduino IDE Files

The original Arduino IDE project files are preserved in the `BitsyMinerOpenSource/` directory for reference. You can safely remove this directory once you've verified the PlatformIO build works correctly.

## Troubleshooting

### Serial Monitor Issues
If serial monitor doesn't connect, ensure the correct port is selected:
```bash
pio device list
```

### TFT Display Issues
Verify the correct environment is selected for your hardware. The display pins and driver are configured per environment in platformio.ini.

### Library Conflicts
PlatformIO manages libraries separately from Arduino IDE. If you have issues, try:
```bash
pio pkg update
```

## Additional Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [PlatformIO ESP32 Platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
