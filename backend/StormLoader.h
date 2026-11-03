#pragma once

#include "StormEvent.h"
#include <string>
#include <vector>

bool loadEvents(const std::string& path, std::vector<StormEvent>& events);