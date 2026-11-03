#pragma once

#include "StormEvent.h"
#include <string>
#include <vector>

std::vector<StormEvent> filterEvents(
    const std::vector<StormEvent>& events,
    const std::string& state,
    int year,
    const std::string& eventType
);