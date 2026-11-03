#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "../backend/StormEvent.h"

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
        for (std::size_t i = 0; i < header.size(); ++i) {
            if (header[i] == "EVENT_ID") {
                storm.eventId = std::stoi(row[i]);
            } else if (header[i] == "STATE") {
                storm.state = row[i];
            } else if (header[i] == "EVENT_TYPE") {
                storm.eventType = row[i];
            }
        }
        events.push_back(storm);
    }

    std::cout << "2023 events loaded: " << events.size() << '\n';
    return 0;

}