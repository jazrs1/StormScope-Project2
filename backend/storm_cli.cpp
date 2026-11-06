
#include "StormFilter.h"
#include "StormLoader.h"
#include "StormSort.h"

#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {


std::string jsonString(const std::string& text) {
    std::string out = "\"";
    char buf[16];

    auto writeUnit = [&](unsigned int unit) {
        std::snprintf(buf, sizeof(buf), "\\u%04x", unit);
        out += buf;
    };
    auto writeCodePoint = [&](unsigned int cp) {
        if (cp >= 0x10000) {
            cp -= 0x10000;
            writeUnit(0xD800 + (cp >> 10));
            writeUnit(0xDC00 + (cp & 0x3FF));
        } else {
            writeUnit(cp);
        }
    };

    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[i]);

        if (c < 0x80) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20) writeUnit(c);
                    else out += static_cast<char>(c);
            }
            ++i;
            continue;
        }


        std::size_t length = 0;
        unsigned int cp = 0;
        if (c >= 0xC2 && c <= 0xDF) { length = 2; cp = c & 0x1F; }
        else if (c >= 0xE0 && c <= 0xEF) { length = 3; cp = c & 0x0F; }
        else if (c >= 0xF0 && c <= 0xF4) { length = 4; cp = c & 0x07; }

        bool valid = length > 0 && i + length <= text.size();
        for (std::size_t k = 1; valid && k < length; ++k) {
            const unsigned char next = static_cast<unsigned char>(text[i + k]);
            if ((next & 0xC0) != 0x80) valid = false;
            else cp = (cp << 6) | (next & 0x3F);
        }
        if (valid && ((length == 3 && cp < 0x800) || (length == 4 && (cp < 0x10000 || cp > 0x10FFFF)) ||
                      (cp >= 0xD800 && cp <= 0xDFFF))) {
            valid = false; 
        }

        if (valid) {
            writeCodePoint(cp);
            i += length;
        } else {
            writeUnit(0xFFFD);  
            ++i;
        }
    }

    out += "\"";
    return out;
}

std::string jsonNumber(double value) {
    std::ostringstream out;
    out.precision(15);
    out << value;
    return out.str();
}



struct Options {
    bool optionsMode = false;
    std::string algorithm = "quick";
    std::string field = "damage";
    std::string state;
    int year = 0;
    std::string eventType;           
    long long offset = 0;
    long long limit = 50;
};

bool parseInteger(const std::string& text, long long& value) {
    if (text.empty() || text.size() > 12) return false;
    std::size_t start = (text[0] == '-') ? 1 : 0;
    if (start == text.size()) return false;
    for (std::size_t i = start; i < text.size(); ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
    }
    value = std::stoll(text);
    return true;
}

bool parseArgs(int argc, char* argv[], Options& opts, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];

        if (flag == "--options") {
            opts.optionsMode = true;
            continue;
        }

        if (i + 1 >= argc) {
            error = "missing value after " + flag;
            return false;
        }
        const std::string value = argv[++i];
        long long number = 0;

        if (flag == "--algorithm") {
            if (value != "quick" && value != "merge" && value != "both") {
                error = "--algorithm must be quick, merge, or both";
                return false;
            }
            opts.algorithm = value;
        } else if (flag == "--field") {
            if (value != "damage" && value != "injuries" && value != "date") {
                error = "--field must be damage, injuries, or date";
                return false;
            }
            opts.field = value;
        } else if (flag == "--state") {
            opts.state = value;
        } else if (flag == "--type") {
            opts.eventType = value;
        } else if (flag == "--year") {
            if (!parseInteger(value, number) || number < 0 || number > 9999) {
                error = "--year must be 0 (All) or a four-digit year";
                return false;
            }
            opts.year = static_cast<int>(number);
        } else if (flag == "--offset") {
            if (!parseInteger(value, number) || number < 0) {
                error = "--offset must be a non-negative integer";
                return false;
            }
            opts.offset = number;
        } else if (flag == "--limit") {
            if (!parseInteger(value, number) || number < 1 || number > 500) {
                error = "--limit must be between 1 and 500";
                return false;
            }
            opts.limit = number;
        } else {
            error = "unknown flag " + flag;
            return false;
        }
    }
    return true;
}

SortField toSortField(const std::string& field) {
    if (field == "injuries") return SortField::Injuries;
    if (field == "date") return SortField::Date;
    return SortField::Damage;
}




double timedSort(std::vector<StormEvent>& work, SortField field, bool useQuick) {
    const auto start = std::chrono::steady_clock::now();
    if (useQuick) {
        quickSort(work, field);
    } else {
        mergeSort(work, field);
    }
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

bool isOrdered(const std::vector<StormEvent>& events, SortField field) {
    for (std::size_t i = 1; i < events.size(); ++i) {
        if (comesBefore(events[i], events[i - 1], field)) return false;
    }
    return true;
}

bool sameIdOrder(const std::vector<StormEvent>& a, const std::vector<StormEvent>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].eventId != b[i].eventId) return false;
    }
    return true;
}

bool loadAll(std::vector<StormEvent>& events) {
    return loadEvents("data/storms_2023.csv", events) &&
           loadEvents("data/storms_2024.csv", events);
}



int runOptions() {
    std::vector<StormEvent> events;
    if (!loadAll(events)) return 1;

    std::set<std::string> states;
    std::set<std::string> types;
    std::set<int> years;
    for (const StormEvent& e : events) {
        states.insert(e.state);
        types.insert(e.eventType);
        years.insert(e.year);
    }

    std::cout << "{\"loadedCount\":" << events.size() << ",\"states\":[";
    bool first = true;
    for (const std::string& s : states) {
        std::cout << (first ? "" : ",") << jsonString(s);
        first = false;
    }
    std::cout << "],\"years\":[";
    first = true;
    for (int y : years) {
        std::cout << (first ? "" : ",") << y;
        first = false;
    }
    std::cout << "],\"types\":[";
    first = true;
    for (const std::string& t : types) {
        std::cout << (first ? "" : ",") << jsonString(t);
        first = false;
    }
    std::cout << "]}\n";
    return 0;
}

int runQuery(const Options& opts) {
    std::vector<StormEvent> events;
    if (!loadAll(events)) return 1;

    const SortField field = toSortField(opts.field);
    const std::vector<StormEvent> matches =
        filterEvents(events, opts.state, opts.year, opts.eventType);

    const bool runQuick = opts.algorithm == "quick" || opts.algorithm == "both";
    const bool runMerge = opts.algorithm == "merge" || opts.algorithm == "both";


    std::vector<StormEvent> quickResult;
    std::vector<StormEvent> mergeResult;
    double quickMs = -1.0;
    double mergeMs = -1.0;

    if (runQuick) {
        quickResult = matches;
        quickMs = timedSort(quickResult, field, true);
        if (quickResult.size() != matches.size() || !isOrdered(quickResult, field)) {
            std::cerr << "quickSort produced an invalid ordering\n";
            return 1;
        }
    }
    if (runMerge) {
        mergeResult = matches;
        mergeMs = timedSort(mergeResult, field, false);
        if (mergeResult.size() != matches.size() || !isOrdered(mergeResult, field)) {
            std::cerr << "mergeSort produced an invalid ordering\n";
            return 1;
        }
    }

    bool ordersMatch = true;
    if (runQuick && runMerge) {
        ordersMatch = sameIdOrder(quickResult, mergeResult);
        if (!ordersMatch) {
            std::cerr << "quickSort and mergeSort orders differ\n";
            return 1;
        }
    }


    const std::vector<StormEvent>& ranked = runQuick ? quickResult : mergeResult;
    const std::size_t begin = static_cast<std::size_t>(opts.offset) < ranked.size()
                                  ? static_cast<std::size_t>(opts.offset) : ranked.size();
    std::size_t end = begin + static_cast<std::size_t>(opts.limit);
    if (end > ranked.size()) end = ranked.size();

    std::ostringstream out;
    out << "{\"loadedCount\":" << events.size()
        << ",\"matchedCount\":" << matches.size()
        << ",\"algorithm\":" << jsonString(opts.algorithm)
        << ",\"field\":" << jsonString(opts.field)
        << ",\"filters\":{\"state\":" << jsonString(opts.state)
        << ",\"year\":" << opts.year
        << ",\"type\":" << jsonString(opts.eventType) << "}"
        << ",\"offset\":" << opts.offset
        << ",\"limit\":" << opts.limit
        << ",\"timingsMs\":{\"quick\":" << (runQuick ? jsonNumber(quickMs) : "null")
        << ",\"merge\":" << (runMerge ? jsonNumber(mergeMs) : "null") << "}"
        << ",\"ordersMatch\":" << ((runQuick && runMerge) ? (ordersMatch ? "true" : "false") : "null")
        << ",\"rows\":[";

    for (std::size_t i = begin; i < end; ++i) {
        const StormEvent& e = ranked[i];
        out << (i == begin ? "" : ",")
            << "{\"eventId\":" << e.eventId
            << ",\"state\":" << jsonString(e.state)
            << ",\"location\":" << jsonString(e.countyOrZone)
            << ",\"type\":" << jsonString(e.eventType)
            << ",\"date\":" << jsonString(e.displayDate)
            << ",\"dateKey\":" << e.dateKey
            << ",\"damage\":" << (e.damageKnown ? jsonNumber(e.propertyDamage) : "null")
            << ",\"injuries\":" << e.totalInjuries
            << ",\"description\":" << jsonString(e.description) << "}";
    }
    out << "]}\n";

    std::cout << out.str();
    return 0;
}

}

int main(int argc, char* argv[]) {
    Options opts;
    std::string error;
    if (!parseArgs(argc, argv, opts, error)) {
        std::cerr << "Invalid arguments: " << error << '\n';
        return 2;
    }

    try {
        return opts.optionsMode ? runOptions() : runQuery(opts);
    } catch (const std::exception& ex) {
        std::cerr << "Unexpected error: " << ex.what() << '\n';
        return 1;
    }
}