# openblock-firmata

## Instructions

```bash
git submodule init
git submodule update
cmake -B build -G Ninja
cmake --build build
```

Flash artifacts land in `dist/<board>/`. Build a single board with
`cmake --build build --target arduinoEsp32`. Reset everything (outputs and
submodule patches) with `cmake --build build --target clean-all`.

Supported boards:

- AVR: `arduinoUno`, `arduinoNano`, `arduinoMega2560`
- ESP32 family: `arduinoEsp32`, `arduinoEsp32S3`, `arduinoEsp32C3`, `arduinoEsp32C6`
- ESP8266: `arduinoEsp8266`
- Pico family: `arduinoPico`, `arduinoPicoW` (Pico 2 / Pico 2W disabled — RP2350 not yet supported in `Boards.h`)

Feature toggles (`ENABLE_*`) are configured in `CMakeLists.txt` via
`FIRMATA_BASE_FEATURES` plus per-board `EXTRA_FEATURES` / `DISABLE_FEATURES`,
not in the sketch.

The Pico (rp2040) and ESP8266 cores are not bundled. Install them via `arduino-cli core install` first:

```bash
arduino-cli core install rp2040:rp2040 \
  --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core install esp8266:esp8266 \
  --additional-urls https://arduino.esp8266.com/stable/package_esp8266com_index.json
```
