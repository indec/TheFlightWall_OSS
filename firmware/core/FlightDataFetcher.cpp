/*
Purpose: Orchestrate fetching and enrichment of flight data for display.
Flow:
1) Use BaseStateVectorFetcher to fetch nearby state vectors by geo filter.
2) For each callsign, use BaseFlightFetcher (e.g., AeroAPI) to retrieve FlightInfo.
3) Enrich names via FlightWallFetcher (airline/aircraft display names).
Output: Returns count of enriched flights and fills outStates/outFlights.
*/
#include "core/FlightDataFetcher.h"
#include "config/UserConfiguration.h"
#include "config/TimingConfiguration.h"
#include "adapters/FlightWallFetcher.h"

FlightDataFetcher::FlightDataFetcher(BaseStateVectorFetcher *stateFetcher,
                                     BaseFlightFetcher *flightFetcher)
    : _stateFetcher(stateFetcher), _flightFetcher(flightFetcher) {}

bool FlightDataFetcher::getCached(const String &callsign, FlightInfo &outInfo)
{
    const unsigned long now = millis();
    const unsigned long ttlMs = TimingConfiguration::AEROAPI_CACHE_TTL_SECONDS * 1000UL;
    for (const auto &entry : _cache)
    {
        if (entry.callsign == callsign && (now - entry.cachedAtMs) < ttlMs)
        {
            outInfo = entry.info;
            return true;
        }
    }
    return false;
}

void FlightDataFetcher::putCached(const String &callsign, const FlightInfo &info)
{
    for (auto &entry : _cache)
    {
        if (entry.callsign == callsign)
        {
            entry.info = info;
            entry.cachedAtMs = millis();
            return;
        }
    }

    if (_cache.size() >= TimingConfiguration::AEROAPI_CACHE_MAX_ENTRIES)
    {
        // Bound memory use: evict the oldest entry rather than growing further.
        size_t oldestIdx = 0;
        unsigned long oldestMs = _cache[0].cachedAtMs;
        for (size_t i = 1; i < _cache.size(); ++i)
        {
            if (_cache[i].cachedAtMs < oldestMs)
            {
                oldestMs = _cache[i].cachedAtMs;
                oldestIdx = i;
            }
        }
        _cache.erase(_cache.begin() + oldestIdx);
    }

    _cache.push_back(CacheEntry{callsign, info, millis()});
}

void FlightDataFetcher::pruneExpired()
{
    const unsigned long now = millis();
    const unsigned long ttlMs = TimingConfiguration::AEROAPI_CACHE_TTL_SECONDS * 1000UL;
    for (size_t i = 0; i < _cache.size();)
    {
        if (now - _cache[i].cachedAtMs >= ttlMs)
        {
            _cache.erase(_cache.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}

size_t FlightDataFetcher::fetchFlights(std::vector<StateVector> &outStates,
                                       std::vector<FlightInfo> &outFlights)
{
    outStates.clear();
    outFlights.clear();

    bool ok = _stateFetcher->fetchStateVectors(
        UserConfiguration::CENTER_LAT,
        UserConfiguration::CENTER_LON,
        UserConfiguration::RADIUS_KM,
        outStates);
    if (!ok)
        return 0;

    pruneExpired();

    size_t enriched = 0;
    for (const StateVector &s : outStates)
    {
        if (s.callsign.length() == 0)
        {
            continue;
        }

        FlightInfo info;
        bool haveInfo = getCached(s.callsign, info);

        if (!haveInfo && _flightFetcher->fetchFlightInfo(s.callsign, info))
        {
            FlightWallFetcher fw;
            if (info.operator_icao.length())
            {
                String airlineFull;
                if (fw.getAirlineName(info.operator_icao, airlineFull))
                {
                    info.airline_display_name_full = airlineFull;
                }
            }
            if (info.aircraft_code.length())
            {
                String aircraftShort, aircraftFull;
                if (fw.getAircraftName(info.aircraft_code, aircraftShort, aircraftFull))
                {
                    if (aircraftShort.length())
                    {
                        info.aircraft_display_name_short = aircraftShort;
                    }
                }
            }
            putCached(s.callsign, info);
            haveInfo = true;
        }

        if (haveInfo)
        {
            outFlights.push_back(info);
            enriched++;
        }
    }
    return enriched;
}
