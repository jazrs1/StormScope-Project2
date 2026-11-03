#pragma once
#include <string>

struct StormEvent {
    int eventId = 0;
    std::string state;
    std::string countyOrZone;
    std::string eventType;
    int year = 0;

    std::string displayDate;
    long long dateKey = 0; 

    double propertyDamage = 0.0;
    bool damageKnown = false;
    int totalInjuries = 0;
    std::string description;
};