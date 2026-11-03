#include "StormFilter.h"

std::vector<StormEvent> filterEvents(
    const std::vector<StormEvent>& events,
    const std::string& state,
    int year,
    const std::string& eventType
) {
    std::vector<StormEvent> matches;

    for (const StormEvent& event : events) {
        if (!state.empty() && event.state != state) continue;
        if (year != 0 && event.year != year) continue;
        if (!eventType.empty() && event.eventType != eventType) continue;

        matches.push_back(event);
    }

    return matches;
}