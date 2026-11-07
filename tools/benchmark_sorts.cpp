//fair timing harness for mergeSort and quickSort on the NOAA storm data.

#include "../backend/StormFilter.h"
#include "../backend/StormLoader.h"
#include "../backend/StormSort.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

const std::uint64_t kSeed = 20261104ULL;
const int kRepeats = 5;

using SortFn = void (*)(std::vector<StormEvent>&, SortField);

struct Algorithm {
    std::string name;
    SortFn fn;
};

std::string fieldName(SortField f) {
    switch (f) {
        case SortField::Damage: return "damage";
        case SortField::Injuries: return "injuries";
        case SortField::Date: return "date";
    }
    return "?";
}

//fisher yates with our own index draw.
void portableShuffle(std::vector<StormEvent>& v, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    for (std::size_t i = v.size(); i > 1; --i) {
        std::uint64_t bound = i;
        std::uint64_t limit = UINT64_MAX - (UINT64_MAX % bound);  
        std::uint64_t r;
        do {
            r = rng();
        } while (r >= limit);
        std::size_t j = static_cast<std::size_t>(r % bound);
        std::swap(v[i - 1], v[j]);
    }
}

//ordered by comesBefore
bool isCorrect(const std::vector<StormEvent>& input,
               const std::vector<StormEvent>& output, SortField field) {
    if (input.size() != output.size()) return false;
    for (std::size_t i = 1; i < output.size(); ++i) {
        if (comesBefore(output[i], output[i - 1], field)) return false;
    }
    std::unordered_map<int, long long> counts;
    for (const StormEvent& e : input) ++counts[e.eventId];
    for (const StormEvent& e : output) --counts[e.eventId];
    for (const auto& pair : counts) {
        if (pair.second != 0) return false;
    }
    return true;
}

//fresh copy of the prepared input then time only the sort call.
double timeOnce(const Algorithm& alg, const std::vector<StormEvent>& input,
                SortField field, bool& correct) {
    std::vector<StormEvent> work = input; 
    auto start = std::chrono::steady_clock::now();
    alg.fn(work, field);
    auto end = std::chrono::steady_clock::now();
    correct = isCorrect(input, work, field);
    return std::chrono::duration<double, std::milli>(end - start).count();
}

struct Row {
    std::string algorithm;
    std::size_t n;
    std::string field;
    std::string order;
    int repeat;
    double ms;
    bool correct;
};

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end()); 
    std::size_t m = v.size() / 2;
    return v.size() % 2 ? v[m] : (v[m - 1] + v[m]) / 2.0;
}

} 

int main() {
    std::vector<StormEvent> events;
    if (!loadEvents("data/storms_2023.csv", events)) return 1;
    if (!loadEvents("data/storms_2024.csv", events)) return 1;
    std::vector<StormEvent> master = filterEvents(events, "", 0, "");
    std::cout << "Loaded " << master.size() << " records (all filters off)\n";
if (master.size() != 145394) {
    std::cerr << "Expected 145394 records for this benchmark, but loaded "
              << master.size()
              << ". Check that both agreed NOAA data files are complete.\n";
    return 1;
}

    const std::vector<Algorithm> algorithms = {{"merge_sort", mergeSort},
                                               {"quick_sort", quickSort}};

    std::vector<StormEvent> shuffled = master;
    portableShuffle(shuffled, kSeed);

    std::vector<std::size_t> sizes = {10000, 50000, 100000, shuffled.size()};
    std::vector<SortField> fields = {SortField::Damage, SortField::Injuries, SortField::Date};
    std::vector<Row> rows;
    bool everythingCorrect = true;

    for (std::size_t n : sizes) {
        std::vector<StormEvent> prefix(shuffled.begin(), shuffled.begin() + n);

        for (SortField field : fields) {
            std::vector<StormEvent> sorted = prefix;
            mergeSort(sorted, field); 
            std::vector<StormEvent> reversed = sorted;
            std::reverse(reversed.begin(), reversed.end());

            std::vector<std::pair<std::string, const std::vector<StormEvent>*>> inputs = {
                {"shuffled", &prefix}, {"sorted", &sorted}, {"reversed", &reversed}};

            for (const auto& input : inputs) {
                for (const Algorithm& alg : algorithms) {
                    bool ok = false;
                    timeOnce(alg, *input.second, field, ok);
                }

                for (int rep = 1; rep <= kRepeats; ++rep) {
                    std::vector<Algorithm> orderThisRep = algorithms;
                    if (rep % 2 == 0) std::reverse(orderThisRep.begin(), orderThisRep.end());

                    for (const Algorithm& alg : orderThisRep) {
                        bool ok = false;
                        double ms = timeOnce(alg, *input.second, field, ok);
                        everythingCorrect = everythingCorrect && ok;
                        rows.push_back({alg.name, n, fieldName(field), input.first, rep, ms, ok});
                    }
                }
                std::cout << "  done n=" << n << " field=" << fieldName(field)
                          << " order=" << input.first << "\n";
            }
        }
    }

    std::ofstream csv("results/sort_benchmarks.csv");
    if (!csv) {
        std::cerr << "Could not open results/sort_benchmarks.csv (does results/ exist?)\n";
        return 1;
    }
    csv << "algorithm,n,field,input_order,repeat,ms,correct,seed\n";
    csv << std::fixed << std::setprecision(3);
    for (const Row& r : rows) {
        csv << r.algorithm << ',' << r.n << ',' << r.field << ',' << r.order << ','
            << r.repeat << ',' << r.ms << ',' << (r.correct ? "true" : "false") << ','
            << kSeed << '\n';
    }

    std::cout << "\n| n | field | input order | merge_sort median ms (min-max) | quick_sort median ms (min-max) |\n";
    std::cout << "|---|---|---|---|---|\n";
    std::cout << std::fixed << std::setprecision(2);
    std::size_t i = 0;
    while (i < rows.size()) {
        std::size_t j = i;
        while (j < rows.size() && rows[j].n == rows[i].n && rows[j].field == rows[i].field &&
               rows[j].order == rows[i].order) {
            ++j;
        }
        std::cout << "| " << rows[i].n << " | " << rows[i].field << " | " << rows[i].order << " |";
        for (const Algorithm& alg : algorithms) {
            std::vector<double> times;
            bool allOk = true;
            for (std::size_t k = i; k < j; ++k) {
                if (rows[k].algorithm == alg.name) {
                    times.push_back(rows[k].ms);
                    allOk = allOk && rows[k].correct;
                }
            }
            std::cout << ' ' << median(times) << " (" << *std::min_element(times.begin(), times.end())
                      << '-' << *std::max_element(times.begin(), times.end()) << ')';
            if (!allOk) std::cout << " FAILED";
            std::cout << " |";
        }
        std::cout << '\n';
        i = j;
    }

    std::cout << "\nRaw results: results/sort_benchmarks.csv\n";
    std::cout << (everythingCorrect ? "Every timed run produced a correct ranking.\n"
                                    : "WARNING: at least one timed run was INCORRECT (see the correct column).\n");
    return everythingCorrect ? 0 : 1;
}