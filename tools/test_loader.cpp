#include "../backend/StormLoader.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
    const char* path = "data/loader_validation_test.tmp";

    // Missing required columns.
    {
        std::ofstream file(path);
        file << "STATE\nFLORIDA\n";
    }

    std::vector<StormEvent> events;
    bool missingRejected = !loadEvents(path, events);
    std::cout << (missingRejected ? "[PASS]" : "[FAIL]")
              << " missing columns rejected\n";

    // Complete header, but an unfinished quoted record.
    {
        std::ofstream file(path);
        file << "EVENT_ID,STATE,EVENT_TYPE,CZ_NAME,YEAR,"
                "BEGIN_YEARMONTH,BEGIN_DAY,BEGIN_TIME,"
                "BEGIN_DATE_TIME,INJURIES_DIRECT,INJURIES_INDIRECT,"
                "DAMAGE_PROPERTY,EVENT_NARRATIVE\n";
        file << "1,\"unfinished";
    }

    events.clear();
    bool malformedRejected = !loadEvents(path, events);
    std::cout << (malformedRejected ? "[PASS]" : "[FAIL]")
              << " unfinished quoted record rejected\n";

    std::remove(path);
    return missingRejected && malformedRejected ? 0 : 1;
}