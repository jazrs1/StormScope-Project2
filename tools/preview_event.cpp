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
int main() {
    std::ifstream file("data/storms_2023.csv");
    if (!file) {
        std::cerr << "Could not open data/storms_2023.csv\n";
        return 1;
    }

       std::vector<std::string> header;
    if (!readCsvRow(file, header)) {
        std::cerr << "Could not read the CSV header.\n";
        return 1;
    }

    std::vector<StormEvent> events;
    std::vector<std::string> row;

    while (readCsvRow(file, row)) {
        if (row.size() != header.size()) {
            std::cerr << "Wrong number of columns in data row "
                      << events.size() + 1 << '\n';
            return 1;
        }

        StormEvent storm;
        int yearMonth = 0;
        int day = 0;
        int time = 0;

        for (std::size_t i = 0; i < header.size(); ++i) {
            if (header[i] == "EVENT_ID") {
                storm.eventId = std::stoi(row[i]);
            } else if (header[i] == "STATE") {
                storm.state = row[i];
            } else if (header[i] == "EVENT_TYPE") {
                storm.eventType = row[i];
            } else if(header[i] == "CZ_NAME"){
                storm.countyOrZone = row[i];
            } else if(header[i] == "YEAR"){
                storm.year = std::stoi(row[i]);
            } else if(header[i] == "INJURIES_DIRECT" || header[i] == "INJURIES_INDIRECT"){
                if(!row[i].empty()){
                    storm.totalInjuries += std::stoi(row[i]);
                }
            } else if(header [i] == "BEGIN_YEARMONTH"){
                yearMonth = std::stoi(row[i]);
            } else if(header[i] == "BEGIN_DAY"){
                day = std::stoi(row[i]);
            } else if(header[i] == "BEGIN_TIME"){
                time = std::stoi(row[i]);
            } else if(header[i] == "BEGIN_DATE_TIME"){
                storm.displayDate = row[i];
            } else if(header[i] == "DAMAGE_PROPERTY"){
                storm.damageKnown = parseDamage(row[i], storm.propertyDamage);
            } else if(header[i] == "EVENT_NARRATIVE"){
                storm.description = row[i];
            }
            
        }
        storm.dateKey = (static_cast<long long>(yearMonth) * 100 + day) * 10000 + time;
        events.push_back(storm);
    }
    for (const StormEvent& event : events) {
    if (event.totalInjuries > 0) {
        std::cout << event.eventId << " | Total injuries: "
                  << event.totalInjuries << '\n';
        break;
    }
}
    std::cout << "2023 events loaded: " << events.size() << '\n';
    for (const StormEvent& event : events) {
    if (event.damageKnown && event.propertyDamage > 0) {
        std::cout << event.eventId << " | Damage: $"
                  << event.propertyDamage << '\n';
        std::cout << "Description: " << event.description << '\n';
        break;
    }
}
    if (!events.empty()) {
    const StormEvent& first = events.front();
    std::cout << first.displayDate << " | " << first.dateKey << '\n';
    std::cout << first.eventId << " | " << first.countyOrZone
              << " | " << first.year << '\n';
}
    return 0;

}