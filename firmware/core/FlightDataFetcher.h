#pragma once

#include <Arduino.h>
#include <vector>
#include "interfaces/BaseStateVectorFetcher.h"
#include "interfaces/BaseFlightFetcher.h"
#include "models/StateVector.h"
#include "models/FlightInfo.h"

class FlightDataFetcher
{
public:
    FlightDataFetcher(BaseStateVectorFetcher *stateFetcher,
                      BaseFlightFetcher *flightFetcher);

    size_t fetchFlights(std::vector<StateVector> &outStates,
                        std::vector<FlightInfo> &outFlights);

private:
    struct CacheEntry
    {
        String callsign;
        FlightInfo info;
        unsigned long cachedAtMs;
    };

    BaseStateVectorFetcher *_stateFetcher;
    BaseFlightFetcher *_flightFetcher;
    std::vector<CacheEntry> _cache;

    bool getCached(const String &callsign, FlightInfo &outInfo);
    void putCached(const String &callsign, const FlightInfo &info);
    void pruneExpired();
};
