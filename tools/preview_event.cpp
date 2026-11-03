#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "../backend/StormEvent.h"
#include <sstream>

bool readCsvRow(std::istream& input, std::vector<std::string>& row) {
    row.clear();
    std::string field;
    bool inQuotes = false;
    char ch;

    while (input.get(ch)) {
        if (ch == '"') {
            if (inQuotes && input.peek() == '"') {
                input.get();           // Two quotes represent one literal quote.
                field += '"';
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            row.push_back(field);
            field.clear();
        } else if (ch == '\n' && !inQuotes) {
            row.push_back(field);
            return true;
        } else {
            field += ch;
        }
    }

    if (inQuotes || (row.empty() && field.empty())) return false;
    row.push_back(field);              // Last row may have no newline.
    return true;
}
bool parseDamage(const std::string& text, double& dollars) {
    std::istringstream input(text);
    double amount;
    if (!(input >> amount) || amount < 0) return false;

    char suffix;
    if (input >> suffix) {
        switch (suffix) {
            case 'K': case 'k': amount *= 1000; break;
            case 'M': case 'm': amount *= 1000000; break;
            case 'B': case 'b': amount *= 1000000000; break;
            default: return false;
        }

        char extra;
        if (input >> extra) return false;
    }

    dollars = amount;
    return true;
}
bool loadEvents(const std::string& path, std::vector<StormEvent>& events) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not open " << path << '\n';
        return false;
    }

    std::vector<std::string> header;
    if (!readCsvRow(file, header)) {
        std::cerr << "Could not read the header in " << path << '\n';
        return false;
    }

    std::vector<std::string> row;
    std::size_t rowNumber = 0;

    while (readCsvRow(file, row)) {
        ++rowNumber;
        if (row.size() != header.size()) {
            std::cerr << "Wrong number of columns in " << path
                      << ", data row " << rowNumber << '\n';
            return false;
        }

        StormEvent storm;
        int yearMonth = 0;
        int day = 0;
        int time = 0;

        for (std::size_t i = 0; i < header.size(); ++i) {
            const std::string& column = header[i];
            const std::string& value = row[i];

            if (column == "EVENT_ID") {
                storm.eventId = std::stoi(value);
            } else if (column == "STATE") {
                storm.state = value;
            } else if (column == "EVENT_TYPE") {
                storm.eventType = value;
            } else if (column == "CZ_NAME") {
                storm.countyOrZone = value;
            } else if (column == "YEAR") {
                storm.year = std::stoi(value);
            } else if (column == "BEGIN_YEARMONTH") {
                yearMonth = std::stoi(value);
            } else if (column == "BEGIN_DAY") {
                day = std::stoi(value);
            } else if (column == "BEGIN_TIME") {
                time = std::stoi(value);
            } else if (column == "BEGIN_DATE_TIME") {
                storm.displayDate = value;
            } else if (column == "INJURIES_DIRECT" ||
                       column == "INJURIES_INDIRECT") {
                if (!value.empty()) {
                    storm.totalInjuries += std::stoi(value);
                }
            } else if (column == "DAMAGE_PROPERTY") {
                storm.damageKnown = parseDamage(value, storm.propertyDamage);
            } else if (column == "EVENT_NARRATIVE") {
                storm.description = value;
            }
        }

        storm.dateKey = (static_cast<long long>(yearMonth) * 100 + day)
                        * 10000 + time;
        events.push_back(storm);
    }

    return true;
}
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

    return 0;
}