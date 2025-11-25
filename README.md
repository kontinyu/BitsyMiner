# BitsyMiner Open Source

BitsyMiner is a Bitcoin lottery miner application designed to run on ESP32 microcontroller devices.

### Required Hardware

BitsyMiner was first programmed to run specifically on "Cheap Yellow Display" boards with 2.8" ILI9341 displays. I have since gotten it working on ST7789 displays, as well as the 2.4" ILI9341 boards, which have a slightly different pin configuration.

You can also compile and run the code in headless mode (no display), which should work on boards that fall under the ESP32 Dev Module category.

The current code includes inline assembly that is very hardware-dependent and is not compatible with other board types.

### Installation

**Option 1:** Compile from Source

Set up your environment by installing all of the required libraries in the Arduino IDE, attach your device, compile, and install.

**Option 2:** Install from Binaries

I have included a few binaries in the "binaries" folder. I may provide a loader at some point, but for the time being, navigate to the [binaries](binaries) folder and follow the instructions there.


### Programming Environment

BitsyMiner started as a personal project to learn more about Bitcoin mining. For simplicity's sake, I began working in the [Arduino IDE](https://www.arduino.cc/en/software/), and I never left it.

I use version 2.0.17 of the Arduino Core for esp32 boards. I ran into odd problems when trying to upgrade, so I just stuck with it.

### Required Libraries
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

## Support

The binaries and code are offered as-is. No support or guarantee of any kind is available for the BitsyMiner Open Source.


## License

BitsyMiner Open Source is licensed under the GNU General Public License v3.0 (GPL-3.0).

Commercial licenses for integrating BitsyMiner into proprietary products or for using the
Enhanced / Pro edition are available. For details, contact: bitsyminer@protonmail.com
