#pragma once

#include <Arduino.h>

namespace TimingConfiguration
{
    // Fetch cadence (seconds) limited by 4000 monthly requests to OpenSky
    static const uint32_t FETCH_INTERVAL_SECONDS = 120; // seconds

    // Display cycling configuration
    static const uint32_t DISPLAY_CYCLE_SECONDS = 3; // seconds per flight when multiple flights

    // AeroAPI lookups are billed per call, so results are cached by callsign
    // for this long before being re-fetched. Route/airline/aircraft type
    // don't change mid-flight, so a long TTL is safe; set generously to
    // cover pattern work/loitering near a local airport (touch-and-goes,
    // student pilots circling) without re-billing the same callsign.
    static const uint32_t AEROAPI_CACHE_TTL_SECONDS = 60UL * 60UL; // 1 hour

    // Upper bound on distinct cached callsigns, so a busy airspace can't
    // grow the cache unboundedly on a memory-constrained ESP32.
    static const size_t AEROAPI_CACHE_MAX_ENTRIES = 64;
}
