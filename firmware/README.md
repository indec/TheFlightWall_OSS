# TheFlightWall Firmware

This is a high-level overview of the firmware that powers TheFlightWall on ESP32.

> **Hardware variant note:** This fork replaces the stock 20-panel WS2812B
> LED-matrix wall with a single small ST7789 SPI IPS TFT. The data-fetching
> pipeline (OpenSky, AeroAPI, FlightWall CDN lookup) is unchanged across
> variants; only the display adapter and hardware config differ. Two board
> targets are defined in `platformio.ini`, selected with `-e <env>`:
>
> | PlatformIO env  | Board                | Panel                        | Default |
> |------------------|-----------------------|-------------------------------|---------|
> | `esp32dev`       | ESP-WROOM-32 dev board | Waveshare 2" ST7789V, 240x320 | yes |
> | `esp32s3_154`    | ESP32-S3 Super Mini    | Generic 1.54" ST7789, 240x240 | no |
>
> `pio run` / `pio run -t upload` build the default (`esp32dev`). For the
> other board: `pio run -e esp32s3_154 -t upload`. See **Wiring** below for
> both.

### What it does
- **Fetch nearby aircraft** from OpenSky Network using OAuth (states/all) filtered by location, radius, and bearing.
- **Enrich flights** with readable airline/aircraft info from AeroAPI and TheFlightWall CDN.
- **Render** a clean, bordered flight card (airline / route / aircraft) on the TFT.

### Key components
- **src/main.cpp**: Entry point. Initializes serial, Wi‑Fi, fetchers, and display. Periodically fetches/enriches and renders.
- **core/FlightDataFetcher**: Orchestrates: fetch state vectors → fetch flight metadata → enrich names.
- **adapters/OpenSkyFetcher**: Queries OpenSky states/all with OAuth; parses and filters by geo.
- **adapters/AeroAPIFetcher**: Retrieves flight details by ident via AeroAPI.
- **adapters/FlightWallFetcher**: Looks up human‑friendly airline/aircraft names from CDN.
- **adapters/TFTDisplay**: Draws a bordered flight card (airline; origin/destination codes stacked with a chevron between them; aircraft type) on the ST7789 TFT, sized to whatever panel dimensions are configured; cycles flights; shows loading/status messages; PWM-dims the backlight.
- **config/**: User/API/timing/hardware/Wi‑Fi settings.
- **models/**: Lightweight structs for `StateVector`, `FlightInfo`, `AirportInfo`.
- **utils/GeoUtils.h**: Haversine distance and bounding boxes.

### Configuration quickstart
- Set Wi‑Fi in `config/WiFiConfiguration.h`.
- Set location and display preferences in `config/UserConfiguration.h`.
- Set intervals in `config/TimingConfiguration.h`.
- Set display pins/dimensions/rotation in `config/HardwareConfiguration.h`.
- Provide API credentials/URLs in `config/APIConfiguration.h` (OpenSky OAuth, AeroAPI key, CDN base).

### Build
- PlatformIO project: see `platformio.ini`.

### Notes
- OpenSky OAuth is required for `states/all`. Token auto‑refreshes with a safety skew.
- Display uses `Adafruit_ST7789`/`Adafruit_GFX` over hardware SPI. Adjust pins/rotation in `config/HardwareConfiguration.h`.
- If the colors look inverted on first boot, call `tft->invertDisplay(true)` in `TFTDisplay::initialize()` — some ST7789 panel batches need it, some don't.
- Panel GRAM col/row offsets (needed because the ST7789 controller's internal
  RAM is physically 240x320 even on a 240x240 glass) are computed
  automatically by `Adafruit_ST7789::init()` for both panel sizes used here —
  nothing to configure. The library's own override hook for this
  (`setColRowStart`) is `protected`, so if a specific clone panel ever needs a
  non-standard offset, that'd require subclassing `Adafruit_ST7789` rather
  than a config value.

## Wiring

### esp32dev: ESP-WROOM-32 + Waveshare 2" (240x320)

Using the ESP32's default VSPI pins for SCK/MOSI. All Dupont, no soldering required.

| Waveshare pin | ESP32 pin      | Notes                                   |
|---------------|-----------------|------------------------------------------|
| VCC           | 3V3             | Do **not** use 5V — logic/panel is 3.3V  |
| GND           | GND             |                                            |
| DIN           | GPIO23 (VSPI MOSI) |                                         |
| CLK           | GPIO18 (VSPI SCK)  |                                         |
| CS            | GPIO5           |                                            |
| DC            | GPIO27          |                                            |
| RST           | GPIO33          |                                            |
| BL            | GPIO32          | PWM-dimmed via `UserConfiguration::DISPLAY_BRIGHTNESS` |

Notes:
- MISO is not connected — the panel is write-only, so the ESP32's GPIO19 is unused.
- These pins avoid the ESP32 boot-strapping pins (GPIO0, 2, 12, 15) and the
  input-only pins (GPIO34–39), so they're safe to drive at boot on a stock
  ESP-WROOM-32 dev board.
- The panel is 240×320 natively (portrait); firmware sets `setRotation(3)` for
  landscape. If the image comes up upside-down, change `TFT_ROTATION` to `1`
  in `HardwareConfiguration.h`.

### esp32s3_154: ESP32-S3 Super Mini + generic 1.54" ST7789 (240x240)

Pins match a proven-working config for this exact board+panel combo from a
different project (BambuHelper, `src/display_ui.cpp` `LGFX_S3`) — not guessed
from a datasheet. ESP32-S3 has no fixed default hardware-SPI pin mapping (any
GPIO can be routed to any SPI signal via the GPIO matrix), so these are
explicitly configured in `TFTDisplay::initialize()` via `SPI.begin(sck, miso,
mosi, cs)` rather than relying on a default pin set.

| Panel pin | ESP32-S3 pin | Notes                                   |
|-----------|--------------|-------------------------------------------|
| VCC       | 3V3          | Do **not** use 5V — logic/panel is 3.3V    |
| GND       | GND          |                                             |
| DIN/SDA   | GPIO11       | SPI2 (FSPI) MOSI                           |
| CLK/SCL   | GPIO12       | SPI2 (FSPI) SCK                            |
| CS        | GPIO10       | Omit if your panel is a 7-pin no-CS module (common on this panel size) — tie CS low on the panel side instead |
| DC        | GPIO9        |                                             |
| RST       | GPIO8        |                                             |
| BLK       | GPIO13       | PWM-dimmed via `UserConfiguration::DISPLAY_BRIGHTNESS` |

Notes:
- MISO is unused (write-only panel) and deliberately not wired, freeing GPIO13 for backlight.
- Avoid GPIO0 (BOOT button), GPIO3/45/46 (strapping pins), and GPIO19/20 (native USB D-/D+) for anything else on this board.
- GPIO26–32 (and GPIO33–37 on PSRAM variants with octal/OPI PSRAM) are wired internally to in-package flash/PSRAM and generally aren't broken out on the header at all — don't plan wiring around them.
- First upload may require holding the BOOT button while plugging in USB if auto-reset doesn't kick in; this is a known quirk of the native-USB-CDC boards (no CP2102/CH340 bridge).
- Panel is square, so `TFT_ROTATION` only changes orientation, not dimensions. Adjust 0–3 in `HardwareConfiguration.h` depending on how it's mounted.
- `pio run -e esp32s3_154` targets `board = lolin_s3_mini`, which matches this chip (ESP32-S3FH4R2: 4MB flash, 2MB QSPI PSRAM) closely enough that it's already proven working on this exact unit via BambuHelper.