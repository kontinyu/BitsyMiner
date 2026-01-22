# BitsyMiner - PlatformIO Setup Guide

## Prerequisites

### 1. Install PlatformIO

#### Option A: Using VS Code (Recommended)
1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Open VS Code
3. Go to Extensions (Ctrl+Shift+X)
4. Search for "PlatformIO IDE"
5. Install the extension
6. Restart VS Code

#### Option B: Using PlatformIO Core (CLI)
```bash
# Using Python pip
pip install platformio

# Verify installation
pio --version
```

## Quick Start

### 1. Open Project in VS Code
```bash
cd c:\Users\konti\ESPStuff\BitsyMiner
code .
```

### 2. Select Build Environment
The project has 4 build environments in `platformio.ini`:
- `esp32_2432s028` (default) - 2.8" ILI9341 display
- `esp32_2432s028_st7789` - 2.8" ST7789 display
- `esp32_2432s024` - 2.4" ILI9341 display
- `esp32_dev_headless` - No display

### 3. Build the Project

#### Using VS Code PlatformIO Extension:
- Click on the PlatformIO icon in the sidebar
- Select "Build" under your chosen environment
- Or press Ctrl+Alt+B for default environment

#### Using Command Line:
```bash
# Build default environment
pio run

# Build specific environment
pio run -e esp32_2432s028
pio run -e esp32_2432s024
pio run -e esp32_dev_headless
```

### 4. Upload to Device

#### Using VS Code:
- Connect your ESP32 via USB
- Click "Upload" in PlatformIO sidebar
- Or press Ctrl+Alt+U

#### Using Command Line:
```bash
# Upload to device
pio run --target upload

# Upload specific environment
pio run -e esp32_2432s028 --target upload
```

### 5. Monitor Serial Output

#### Using VS Code:
- Click "Monitor" in PlatformIO sidebar
- Or press Ctrl+Alt+S

#### Using Command Line:
```bash
pio device monitor

# With custom baud rate
pio device monitor -b 115200
```

## Project Structure

```
BitsyMiner/
├── platformio.ini              # PlatformIO configuration
├── src/                        # Main source files
│   ├── main.cpp               # Main application (was .ino)
│   ├── defines_n_types.h      # Project definitions
│   ├── display.*              # Display handling
│   ├── miner.*                # Mining functionality
│   ├── stratum.*              # Stratum protocol
│   ├── MyWiFi.*               # WiFi management
│   └── ...                    # Other source files
├── lib/                        # Custom libraries
├── include/                    # Additional headers
├── PLATFORMIO_MIGRATION.md    # Migration details
└── PLATFORMIO_SETUP.md        # This file
```

## Common Tasks

### Clean Build
```bash
pio run --target clean
```

### Update Dependencies
```bash
pio pkg update
```

### List Connected Devices
```bash
pio device list
```

### Build All Environments
```bash
pio run -e esp32_2432s028
pio run -e esp32_2432s028_st7789
pio run -e esp32_2432s024
pio run -e esp32_dev_headless
```

## Hardware-Specific Builds

### For ESP32-2432S028 (2.8" ILI9341)
```bash
pio run -e esp32_2432s028 --target upload
pio device monitor
```

### For ESP32-2432S028 with ST7789
```bash
pio run -e esp32_2432s028_st7789 --target upload
pio device monitor
```

### For ESP32-2432S024 (2.4")
```bash
pio run -e esp32_2432s024 --target upload
pio device monitor
```

### For Headless ESP32
```bash
pio run -e esp32_dev_headless --target upload
pio device monitor
```

## Debugging

### Enable Debug Messages
Debug messages are enabled by default via `-DDEBUG_MESSAGES` flag in the code.

### View Build Output
```bash
# Verbose build
pio run -v

# Very verbose
pio run -vv
```

### Check Errors
If you encounter build errors:
1. Clean the build: `pio run --target clean`
2. Update libraries: `pio pkg update`
3. Check platform: `pio platform update espressif32`

## Configuration

### Change Default Environment
Edit `platformio.ini`:
```ini
[platformio]
default_envs = esp32_2432s028  # Change this line
```

### Modify Build Flags
Each environment in `platformio.ini` has its own `build_flags` section where you can add custom defines:
```ini
[env:esp32_2432s028]
build_flags = 
    -DESP32_2432S028
    -DDEBUG_MESSAGES
    # Add more flags here
```

### Add New Library
Edit `platformio.ini` under `[env]` section:
```ini
lib_deps = 
    bblanchon/ArduinoJson@^7.0.0
    bodmer/TFT_eSPI@^2.5.43
    your/NewLibrary@^1.0.0  # Add here
```

Or use command line:
```bash
pio pkg install --library "author/LibraryName@^version"
```

## VS Code Integration Features

- **IntelliSense**: Full code completion and navigation
- **Integrated Terminal**: Access PlatformIO commands
- **Task Runner**: Pre-configured build/upload/monitor tasks
- **Problem Matcher**: Compiler errors shown inline
- **Serial Monitor**: Built into VS Code

## Troubleshooting

### "pio: command not found"
- If using VS Code: Install PlatformIO IDE extension
- If using CLI: Install PlatformIO Core via pip
- Add to PATH: Typically `~/.platformio/penv/bin` or `C:\Users\<user>\.platformio\penv\Scripts`

### Serial Port Access Issues
**Windows**: Install [CH340 or CP210x drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

### Upload Fails
1. Check device is connected: `pio device list`
2. Hold BOOT button during upload for some ESP32 boards
3. Try different USB cable or port

### TFT Display Not Working
1. Verify correct environment for your hardware
2. Check build flags match your display type
3. Ensure correct pins in `platformio.ini`

### Out of Memory During Build
Increase available memory in `platformio.ini`:
```ini
build_flags = 
    -DBOARD_HAS_PSRAM
```

## Migration from Arduino IDE

Key differences:
1. **No .ino files**: Use `main.cpp` instead
2. **Automatic library management**: Defined in `platformio.ini`
3. **Multiple build configurations**: One project, many targets
4. **Better dependency handling**: Automatic version management
5. **Professional toolchain**: GCC, GDB debugging support

## Additional Resources

- [PlatformIO Documentation](https://docs.platformio.org/)
- [ESP32 Platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)
- [PlatformIO Core CLI](https://docs.platformio.org/en/latest/core/index.html)
- [Troubleshooting Guide](https://docs.platformio.org/en/latest/faq.html)

## Support

For BitsyMiner-specific issues, refer to the main README.md or project documentation.
For PlatformIO issues, visit: https://community.platformio.org/
