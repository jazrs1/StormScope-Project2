#include "../backend/StormLoader.h"
#include "../backend/StormFilter.h"
#include <iostream>

int main() {
    std::vector<StormEvent> events;

    if (!loadEvents("data/storms_2023.csv", events)) {
        return 1;
    }

const std::size_t count2023 = events.size();
std::cout << "2023 events loaded: " << count2023 << '\n';

if (!loadEvents("data/storms_2024.csv", events)) {
    return 1;
}

std::cout << "2024 events loaded: " << events.size() - count2023 << '\n';
std::cout << "Total events loaded: " << events.size() << '\n';
    if (!events.empty()) {
        const StormEvent& first = events.front();
        std::cout << first.displayDate << " | " << first.dateKey << '\n';
        std::cout << first.eventId << " | " << first.countyOrZone
                  << " | " << first.year << '\n';
    }

    for (const StormEvent& event : events) {
        if (event.totalInjuries > 0) {
            std::cout << event.eventId << " | Total injuries: "
                      << event.totalInjuries << '\n';
            break;
        }
    }

    for (const StormEvent& event : events) {
        if (event.damageKnown && event.propertyDamage > 0) {
            std::cout << event.eventId << " | Damage: $"
                      << event.propertyDamage << '\n';
            std::cout << "Description: " << event.description << '\n';
            break;
        }
    }
    const auto floods = filterEvents(events, "FLORIDA", 2024, "Flood");
    std::cout << "Florida floods in 2024: " << floods.size() << '\n';

    const auto all = filterEvents(events, "", 0, "");
    std::cout << "All filters off: " << all.size() << '\n';
    return 0;
}