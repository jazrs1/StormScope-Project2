#include "../backend/StormFilter.h"
#include "../backend/StormLoader.h"
#include "../backend/StormSort.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

std::string fieldName(SortField f) {
    switch (f) {
        case SortField::Damage: return "Damage";
        case SortField::Injuries: return "Injuries";
        case SortField::Date: return "Date";
    }
    return "?";
}

std::string keyText(const StormEvent& e, SortField f) {
    switch (f) {
        case SortField::Damage:
            if (!e.damageKnown) return "damage unknown";
            {
                std::ostringstream out;
                out << "$" << std::fixed << std::setprecision(0) << e.propertyDamage;
                return out.str();
            }
        case SortField::Injuries:
            return std::to_string(e.totalInjuries) + " injuries";
        case SortField::Date:
            return e.displayDate + " (" + std::to_string(e.dateKey) + ")";
    }
    return "";
}

//full-output checks: order of every adjacent pair, count, and ID frequencies
bool validate(const std::vector<StormEvent>& before,
              const std::vector<StormEvent>& after, SortField field) {
    bool ok = true;

    if (before.size() != after.size()) {
        std::cout << "  FAIL: size changed " << before.size() << " -> " << after.size() << "\n";
        ok = false;
    }

    for (std::size_t i = 1; i < after.size(); ++i) {
        if (comesBefore(after[i], after[i - 1], field)) {
            std::cout << "  FAIL: out of order at index " << i << "\n";
            ok = false;
            break;
        }
    }

    std::unordered_map<int, long long> counts;
    for (const StormEvent& e : before) ++counts[e.eventId];
    for (const StormEvent& e : after) --counts[e.eventId];
    for (const auto& pair : counts) {
        if (pair.second != 0) {
            std::cout << "  FAIL: ID " << pair.first << " count changed\n";
            ok = false;
            break;
        }
    }
    return ok;
}

bool runOne(const std::string& label, const std::vector<StormEvent>& filtered, SortField field) {
    std::vector<StormEvent> work = filtered;
    mergeSort(work, field);

    std::cout << label << " | " << fieldName(field) << " | " << work.size() << " records\n";
    for (std::size_t i = 0; i < work.size() && i < 5; ++i) {
        std::cout << "  " << (i + 1) << ". " << work[i].eventId << "  "
                  << keyText(work[i], field) << "  " << work[i].state << ", "
                  << work[i].countyOrZone << " (" << work[i].eventType << ")\n";
    }

    bool ok = validate(filtered, work, field);
    std::cout << "  " << (ok ? "PASS" : "FAIL") << ": whole output checked\n\n";
    return ok;
}

} 

int main() {
    std::vector<StormEvent> events; 
    if (!loadEvents("data/storms_2023.csv", events)) return 1;
    if (!loadEvents("data/storms_2024.csv", events)) return 1;
    std::cout << "Total events loaded: " << events.size() << "\n\n";

    bool allOk = true;
    if (events.size() != 145394) {
        std::cout << "WARNING: expected 145394 records for the agreed data version\n\n";
        allOk = false;
    }

    const std::vector<StormEvent> floods = filterEvents(events, "FLORIDA", 2024, "Flood");
    const std::vector<StormEvent> all = filterEvents(events, "", 0, "");

    for (SortField f : {SortField::Damage, SortField::Injuries, SortField::Date}) {
        allOk = runOne("Florida Flood 2024", floods, f) && allOk;
    }
    for (SortField f : {SortField::Damage, SortField::Injuries, SortField::Date}) {
        allOk = runOne("All filters off", all, f) && allOk;
    }

    std::cout << (allOk ? "All real-data checks passed.\n" : "Some real-data checks FAILED.\n");
    return allOk ? 0 : 1;
}