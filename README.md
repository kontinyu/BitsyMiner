# BitsyMiner Open Source

BitsyMiner is a Bitcoin lottery miner application designed to run on ESP32 microcontroller devices.

![Image of BitsyMiner Screen](/assets/bitsy_image.jpg)

<br/><br/>
### Required Hardware

BitsyMiner was first programmed to run specifically on "Cheap Yellow Display" boards with 2.8" ILI9341 displays. I have since gotten it working on ST7789 displays, as well as the 2.4" ILI9341 boards, which have a slightly different pin configuration.

You can also compile and run the code in headless mode (no display), which should work on boards that fall under the ESP32 Dev Module category.

The current code includes inline assembly that is very hardware-dependent and is not compatible with other board types.


<br/><br/>
### Installation

> **🚀 PLATFORMIO MIGRATION**: This project now uses PlatformIO for better dependency management and multi-environment builds. See [PLATFORMIO_SETUP.md](PLATFORMIO_SETUP.md) for quick start guide.

**Option 1:** Compile with PlatformIO (Recommended)

1. Install [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode) for VS Code
2. Open this project folder in VS Code
3. Select your hardware environment in `platformio.ini` (default: esp32_2432s028)
4. Click "Build" or "Upload" in the PlatformIO sidebar

See [PLATFORMIO_SETUP.md](PLATFORMIO_SETUP.md) for detailed instructions.

**Option 2:** Compile from Source (Arduino IDE - Legacy)

The original Arduino IDE project files are in the `BitsyMinerOpenSource/` folder.

Set up your environment by installing all of the required libraries in the Arduino IDE, attach your device, compile, and install.

Changes to display type can be set in defines_n_types.h.

In the tools menu, change the settings as follow:

- Arduino Runs on Core 0
- Events Run on Core 0
- Parition Scheme:  Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)


**Option 3:** Install from Binaries

I have included a few binaries in the "binaries" folder. I may provide a loader at some point, but for the time being, navigate to the [binaries](binaries) folder and follow the instructions there.

After installing, you can follow the setup video [here](https://www.youtube.com/watch?v=Ur3amBXdaBI).

<br/><br/>
### Programming Environment

BitsyMiner started as a personal project to learn more about Bitcoin mining. For simplicity's sake, I began working in the [Arduino IDE](https://www.arduino.cc/en/software/).

**The project has now been migrated to PlatformIO** for better dependency management, multi-environment builds, and professional development tooling. Both Arduino IDE (legacy) and PlatformIO projects are maintained in this repository.


<br/><br/>
### Required Libraries (PlatformIO - Automatic)

When using PlatformIO, all dependencies are automatically managed via `platformio.ini`:

- **ArduinoJson** (^7.0.0) - JSON parsing
- **TFT_eSPI** (^2.5.43) - Display driver  
- **XPT2046_Touchscreen** (^1.4) - Touch controller
- **PNGdec** (^1.0.2) - PNG decoder

<br/><br/>
### Required Libraries (Arduino IDE - Manual)

ArduinoJson
Copyright © 2014-2024, Benoit BLANCHON
MIT License
https://github.com/bblanchon/ArduinoJson/blob/7.x/LICENSE.txt

NTPClient by Fabrice WeinBerg
MIT License
https://github.com/arduino-libraries/NTPClient

CustomJWT
Antony Jose Kuruvilla
Public Domain
https://github.com/Ant2000/CustomJWT/blob/main/LICENSE

PNGDec
Copyright 2020 BitBank Software, Inc.
Apache License 2.0
https://github.com/bitbank2/PNGdec/blob/master/LICENSE

TFT_eSPI
Bodmer
MIT License
https://github.com/Bodmer/TFT_eSPI/blob/master/license.txt

QRCode
Copyright (c) 2017 Richard Moore
MIT License
https://github.com/ricmoo/QRCode/blob/master/LICENSE.txt

XPT2046_Touchscreen
Paul Stoffregen
No license defined (Public Domain)
https://github.com/PaulStoffregen/XPT2046_Touchscreen    


<br/><br/>
## Support

The binaries and code are offered as-is. No support or guarantee of any kind is available for BitsyMiner Open Source.



<br/><br/>
## License

BitsyMiner Open Source is licensed under the GNU General Public License v3.0 (GPL-3.0).

Commercial licenses for integrating BitsyMiner into proprietary products or for using the
Enhanced / Pro edition are available. For details, contact: bitsyminer@protonmail.com
