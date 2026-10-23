/*
Purpose: Render flight info on an ST7789 SPI TFT via Adafruit_ST7789. Supports
         multiple board/panel combos selected at build time (HardwareConfiguration.h).
Responsibilities:
- Initialize the panel (SPI bus/pins, offsets, rotation, backlight PWM) based
  on HardwareConfiguration.
- Render a bordered flight "card" (airline/route/aircraft) sized to whatever
  panel dimensions are configured, and a minimal loading screen.
- Cycle through multiple flights at a configurable interval.
Inputs: FlightInfo list; UserConfiguration (colors/brightness), TimingConfiguration (cycle),
        HardwareConfiguration (pins/dimensions/rotation).
Outputs: Visual output to the TFT panel via Adafruit_GFX/Adafruit_ST7789.
*/
#include "adapters/TFTDisplay.h"

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "config/UserConfiguration.h"
#include "config/HardwareConfiguration.h"
#include "config/TimingConfiguration.h"

TFTDisplay::TFTDisplay() {}

TFTDisplay::~TFTDisplay()
{
    if (_tft)
    {
        delete _tft;
        _tft = nullptr;
    }
}

bool TFTDisplay::initialize()
{
    if (HardwareConfiguration::TFT_SCK >= 0)
    {
        // No fixed default hardware-SPI pin mapping on this board (e.g.
        // ESP32-S3): configure the bus explicitly before handing it to the
        // display driver.
        SPI.begin(HardwareConfiguration::TFT_SCK,
                  HardwareConfiguration::TFT_MISO,
                  HardwareConfiguration::TFT_MOSI,
                  HardwareConfiguration::TFT_CS);
        _tft = new Adafruit_ST7789(&SPI,
                                    HardwareConfiguration::TFT_CS,
                                    HardwareConfiguration::TFT_DC,
                                    HardwareConfiguration::TFT_RST);
    }
    else
    {
        _tft = new Adafruit_ST7789(HardwareConfiguration::TFT_CS,
                                    HardwareConfiguration::TFT_DC,
                                    HardwareConfiguration::TFT_RST);
    }

    // Adafruit_ST7789::init() computes the correct GRAM col/row start for
    // known panel sizes on its own (e.g. 240x240 needs an 80-row offset
    // since the controller's GRAM is physically 320 rows tall; 240x320 needs
    // none) - setColRowStart() exists to override it but is a protected
    // method, only callable from a subclass, so there's no supported way to
    // override it from here. If a specific clone panel ever needs a
    // non-standard offset, that'd require subclassing Adafruit_ST7789.
    _tft->init(HardwareConfiguration::TFT_PANEL_WIDTH, HardwareConfiguration::TFT_PANEL_HEIGHT);
    _tft->setRotation(HardwareConfiguration::TFT_ROTATION);
    _tft->setTextWrap(false);

    pinMode(HardwareConfiguration::TFT_BL, OUTPUT);
    ledcAttach(HardwareConfiguration::TFT_BL,
               HardwareConfiguration::TFT_BL_PWM_FREQ_HZ,
               HardwareConfiguration::TFT_BL_PWM_RESOLUTION_BITS);
    setBacklight(UserConfiguration::DISPLAY_BRIGHTNESS);

    clear();
    _currentFlightIndex = 0;
    _lastCycleMs = millis();
    return true;
}

void TFTDisplay::setBacklight(uint8_t brightness)
{
    ledcWrite(HardwareConfiguration::TFT_BL, brightness);
}

uint16_t TFTDisplay::textColor() const
{
    return _tft->color565(UserConfiguration::TEXT_COLOR_R,
                           UserConfiguration::TEXT_COLOR_G,
                           UserConfiguration::TEXT_COLOR_B);
}

void TFTDisplay::clear()
{
    if (_tft)
    {
        _tft->fillScreen(ST77XX_BLACK);
    }
}

String TFTDisplay::makeFlightLine(const FlightInfo &f)
{
    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                                                          : (f.operator_iata.length() ? f.operator_iata : f.operator_icao);
    if (airline.length() == 0)
    {
        airline = f.operator_code;
    }
    String origin = f.origin.code_icao;
    String dest = f.destination.code_icao;
    String route = origin + "-" + dest;
    String type = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code;
    String ident = f.ident.length() ? f.ident : f.ident_icao;
    String line = airline;
    if (ident.length())
    {
        line += " ";
        line += ident;
    }
    if (type.length())
    {
        line += " ";
        line += type;
    }
    if (route.length() > 1)
    {
        line += " ";
        line += route;
    }
    return line;
}

String TFTDisplay::truncateToFit(const String &text, uint8_t textSize, int16_t maxWidthPx)
{
    const int16_t charWidthPx = 6 * textSize; // 5x7 glyph + 1px spacing, scaled
    const int maxCols = maxWidthPx / charWidthPx;
    if ((int)text.length() <= maxCols)
        return text;
    if (maxCols <= 3)
        return text.substring(0, maxCols);
    return text.substring(0, maxCols - 3) + String("...");
}

uint8_t TFTDisplay::pickFitSize(size_t textLen, int16_t maxWidthPx, uint8_t maxSize)
{
    uint8_t size = maxSize;
    while (size > 1 && (int16_t)(textLen * 6 * size) > maxWidthPx)
    {
        size--;
    }
    return size;
}

void TFTDisplay::drawCentered(const String &text, int16_t cy, uint8_t textSize, uint16_t color)
{
    const int16_t charWidthPx = 6 * textSize;
    const int16_t charHeightPx = 8 * textSize;
    const int16_t textWidthPx = (int16_t)text.length() * charWidthPx;
    const int16_t x = (HardwareConfiguration::DISPLAY_WIDTH - textWidthPx) / 2;
    const int16_t y = cy - charHeightPx / 2;

    _tft->setTextSize(textSize);
    _tft->setTextColor(color);
    _tft->setCursor(x < 0 ? 0 : x, y < 0 ? 0 : y);
    _tft->print(text);
}

void TFTDisplay::drawChevronDown(int16_t cy, int16_t halfWidth, uint16_t color)
{
    const int16_t cx = HardwareConfiguration::DISPLAY_WIDTH / 2;
    _tft->fillTriangle(cx - halfWidth, cy - halfWidth,
                        cx + halfWidth, cy - halfWidth,
                        cx, cy + halfWidth,
                        color);
}

void TFTDisplay::displaySingleFlightCard(const FlightInfo &f)
{
    const uint16_t color = textColor();
    const int16_t w = HardwareConfiguration::DISPLAY_WIDTH;
    const int16_t h = HardwareConfiguration::DISPLAY_HEIGHT;
    const int16_t margin = 6;

    _tft->fillScreen(ST77XX_BLACK);
    _tft->drawRect(0, 0, w, h, color);
    _tft->drawRect(1, 1, w - 2, h - 2, color);

    // Lines: airline (size 2); origin/destination codes stacked with a
    // chevron between them (the headline - each code gets its own line so
    // it can run much bigger than a single "ORIG > DEST" line would allow,
    // especially on narrower square panels); aircraft type (size 2).
    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                                                          : (f.operator_iata.length() ? f.operator_iata : (f.operator_icao.length() ? f.operator_icao : f.operator_code));

    String origin = f.origin.code_icao.length() ? f.origin.code_icao : String("----");
    String dest = f.destination.code_icao.length() ? f.destination.code_icao : String("----");

    String aircraft = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code;

    const int16_t usableWidth = w - 2 * (margin + 2);
    airline = truncateToFit(airline, 2, usableWidth);
    aircraft = truncateToFit(aircraft, 2, usableWidth);

    // Cap at size 5 for visual balance even when there's room to go bigger
    // (e.g. a short 3-letter IATA code on the wide landscape panel).
    const uint8_t codeSize = min(pickFitSize(origin.length(), usableWidth, 5),
                                 pickFitSize(dest.length(), usableWidth, 5));
    origin = truncateToFit(origin, codeSize, usableWidth);
    dest = truncateToFit(dest, codeSize, usableWidth);

    const int16_t lineGap = 10;
    const int16_t codeGap = 6;
    const int16_t chevronHalf = 7;
    const int16_t airlineH = 16;         // 8px * size 2
    const int16_t codeH = 8 * codeSize;
    const int16_t aircraftH = 16;        // 8px * size 2
    const int16_t totalH = airlineH + lineGap +
                           codeH + codeGap + (chevronHalf * 2) + codeGap +
                           codeH + lineGap + aircraftH;
    int16_t y = (h - totalH) / 2;

    drawCentered(airline, y + airlineH / 2, 2, color);
    y += airlineH + lineGap;
    drawCentered(origin, y + codeH / 2, codeSize, color);
    y += codeH + codeGap;
    drawChevronDown(y + chevronHalf, chevronHalf, color);
    y += chevronHalf * 2 + codeGap;
    drawCentered(dest, y + codeH / 2, codeSize, color);
    y += codeH + lineGap;
    drawCentered(aircraft, y + aircraftH / 2, 2, color);
}

void TFTDisplay::displayFlights(const std::vector<FlightInfo> &flights)
{
    if (_tft == nullptr)
        return;

    if (!flights.empty())
    {
        const unsigned long now = millis();
        const unsigned long intervalMs = TimingConfiguration::DISPLAY_CYCLE_SECONDS * 1000UL;

        if (flights.size() > 1)
        {
            if (now - _lastCycleMs >= intervalMs)
            {
                _lastCycleMs = now;
                _currentFlightIndex = (_currentFlightIndex + 1) % flights.size();
            }
        }
        else
        {
            _currentFlightIndex = 0;
        }

        const size_t index = _currentFlightIndex % flights.size();
        displaySingleFlightCard(flights[index]);
    }
    else
    {
        displayLoadingScreen();
    }
}

void TFTDisplay::displayLoadingScreen()
{
    if (_tft == nullptr)
        return;

    const uint16_t borderColor = ST77XX_WHITE;
    const int16_t w = HardwareConfiguration::DISPLAY_WIDTH;
    const int16_t h = HardwareConfiguration::DISPLAY_HEIGHT;

    _tft->fillScreen(ST77XX_BLACK);
    _tft->drawRect(0, 0, w, h, borderColor);

    drawCentered(String("..."), h / 2, 3, textColor());
}

void TFTDisplay::displayMessage(const String &message)
{
    if (_tft == nullptr)
        return;

    _tft->fillScreen(ST77XX_BLACK);

    const int16_t usableWidth = HardwareConfiguration::DISPLAY_WIDTH - 2 * 6;
    String line = truncateToFit(message, 2, usableWidth);
    drawCentered(line, HardwareConfiguration::DISPLAY_HEIGHT / 2, 2, textColor());
}

void TFTDisplay::showLoading()
{
    displayLoadingScreen();
}
