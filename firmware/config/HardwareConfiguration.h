#pragma once

#include <Arduino.h>

#if !defined(FLIGHTWALL_BOARD_WROOM32_WAVESHARE2) && !defined(FLIGHTWALL_BOARD_S3SUPERMINI_154)
#error "No FlightWall hardware variant selected. Build with the esp32dev or esp32s3_154 PlatformIO environment (see platformio.ini)."
#endif

namespace HardwareConfiguration
{
#if defined(FLIGHTWALL_BOARD_WROOM32_WAVESHARE2)
    // ESP-WROOM-32 dev board + Waveshare 2" IPS TFT (ST7789V, 240x320, SPI).
    // SCK/MOSI use the ESP32's default hardware-SPI (VSPI) pins, so TFT_SCK/
    // TFT_MOSI are left at -1 ("use the default SPI object as-is").
    static const int8_t TFT_CS = 5;
    static const int8_t TFT_DC = 27;
    static const int8_t TFT_RST = 33;
    static const int8_t TFT_BL = 32; // backlight, PWM-dimmable via LEDC
    static const int8_t TFT_SCK = -1;
    static const int8_t TFT_MOSI = -1;
    static const int8_t TFT_MISO = -1; // unused, panel is write-only

    // Panel is 240x320 natively (portrait). TFT_ROTATION below renders it
    // landscape; use 1 or 3 depending on which way the ribbon/header ends up
    // facing once mounted, and flip if the image comes up upside down.
    static const uint16_t TFT_PANEL_WIDTH = 240;
    static const uint16_t TFT_PANEL_HEIGHT = 320;
    static const uint8_t TFT_ROTATION = 3;

    // Logical display dimensions after rotation (landscape).
    static const uint16_t DISPLAY_WIDTH = TFT_PANEL_HEIGHT;  // 320
    static const uint16_t DISPLAY_HEIGHT = TFT_PANEL_WIDTH;  // 240

#elif defined(FLIGHTWALL_BOARD_S3SUPERMINI_154)
    // ESP32-S3 Super Mini + generic 1.54" ST7789 240x240 SPI TFT.
    // Pins/offsets match the proven working config from a different ESP32-S3
    // Super Mini + 240x240 ST7789 project on this same hardware (BambuHelper,
    // src/display_ui.cpp LGFX_S3), not guessed from datasheets. ESP32-S3 has
    // no fixed default hardware-SPI pin mapping, so TFT_SCK/MOSI are set
    // explicitly and TFTDisplay.cpp configures the SPI bus by hand.
    static const int8_t TFT_CS = 10;
    static const int8_t TFT_DC = 9;
    static const int8_t TFT_RST = 8;
    static const int8_t TFT_BL = 13; // backlight, PWM-dimmable via LEDC
    static const int8_t TFT_SCK = 12;
    static const int8_t TFT_MOSI = 11;
    static const int8_t TFT_MISO = -1; // unused, panel is write-only

    // Square panel: rotation doesn't change width/height, only orientation.
    // Adjust 0-3 depending on how the panel ends up mounted/wired.
    static const uint16_t TFT_PANEL_WIDTH = 240;
    static const uint16_t TFT_PANEL_HEIGHT = 240;
    static const uint8_t TFT_ROTATION = 0;

    static const uint16_t DISPLAY_WIDTH = TFT_PANEL_WIDTH;   // 240
    static const uint16_t DISPLAY_HEIGHT = TFT_PANEL_HEIGHT; // 240
#endif

    // LEDC channel/timer used for backlight PWM dimming.
    static const uint32_t TFT_BL_PWM_FREQ_HZ = 5000;
    static const uint8_t TFT_BL_PWM_RESOLUTION_BITS = 8;
}
