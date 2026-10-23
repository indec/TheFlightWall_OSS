#pragma once

#include <Arduino.h>
#include <vector>
#include "interfaces/BaseDisplay.h"

class Adafruit_ST7789;

class TFTDisplay : public BaseDisplay
{
public:
    TFTDisplay();
    ~TFTDisplay() override;

    bool initialize() override;
    void clear() override;
    void displayFlights(const std::vector<FlightInfo> &flights) override;
    void displayMessage(const String &message);
    void showLoading();

private:
    Adafruit_ST7789 *_tft = nullptr;

    size_t _currentFlightIndex = 0;
    unsigned long _lastCycleMs = 0;

    void setBacklight(uint8_t brightness);
    uint16_t textColor() const;
    String makeFlightLine(const FlightInfo &f);
    String truncateToFit(const String &text, uint8_t textSize, int16_t maxWidthPx);
    uint8_t pickFitSize(size_t textLen, int16_t maxWidthPx, uint8_t maxSize);
    void drawCentered(const String &text, int16_t cy, uint8_t textSize, uint16_t color);
    void drawChevronDown(int16_t cy, int16_t halfWidth, uint16_t color);
    void displaySingleFlightCard(const FlightInfo &f);
    void displayLoadingScreen();
};
