# About

This folder contains a [PlatformIO](https://platformio.org/platformio-ide) project to compile the Arduino firmware files in `fw/squidrid` (`./src` is a symbolic link to `../fw/squidrid`). 

PlatformIO is a new generation toolset for embedded C/C++ development as a [VSCode](https://code.visualstudio.com/) extension.

# Configuration

This PlatformIO project is currently configured for a `featheresp32` board in the `arduino` environment. Specifically, I am using [Adafruit Huzzah32 Featherboard](https://www.adafruit.com/product/3405), but any ESP32 board should do. Just change configuration in PlatformIO extension (see `platformio.ini`). 

