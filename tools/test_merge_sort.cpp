#include "../backend/StormSort.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool ok, const std::string& name) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << "\n";
    if (!ok) {
        ++failures;
    }
}

StormEvent makeEvent(int id, bool damageKnown, double damage, int injuries,
                     long long dateKey, const std::string& description = "") {
    StormEvent e;
    e.eventId = id;
    e.state = "FLORIDA";
    e.countyOrZone = "COUNTY_" + std::to_string(id);
    e.eventType = "Flood";
    e.year = static_cast<int>(dateKey / 100000000LL);
    e.displayDate = "date-" + std::to_string(dateKey);
    e.dateKey = dateKey;
    e.damageKnown = damageKnown;
    e.propertyDamage = damage;
    e.totalInjuries = injuries;
    e.description = description.empty() ? "desc-" + std::to_string(id) : description;
    return e;
}

std::vector<StormEvent> guideFixture() {
    return {
        makeEvent(10, true, 0.0, 2, 202301010000LL),
        makeEvent(20, true, 1000.0, 0, 202402010000LL),
        makeEvent(30, false, 0.0, 5, 202312010000LL),
        makeEvent(40, true, 1000.0, 2, 202402010000LL),
    };
}

std::vector<int> ids(const std::vector<StormEvent>& events) {
    std::vector<int> out;
    for (const StormEvent& e : events) {
        out.push_back(e.eventId);
    }
    return out;
}

std::map<int, int> idCounts(const std::vector<StormEvent>& events) {
    std::map<int, int> counts;
    for (const StormEvent& e : events) {
        ++counts[e.eventId];
    }
    return counts;
}

bool isOrdered(const std::vector<StormEvent>& events, SortField field) {
    for (std::size_t i = 1; i < events.size(); ++i) {
        if (comesBefore(events[i], events[i - 1], field)) {
            return false;
        }
    }
    return true;
}

//only valid when IDs are unique
bool fieldsStillAttached(const std::vector<StormEvent>& before,
                         const std::vector<StormEvent>& after) {
    std::map<int, StormEvent> original;
    for (const StormEvent& e : before) {
        original[e.eventId] = e;
    }
    for (const StormEvent& e : after) {
        const StormEvent& o = original[e.eventId];
        if (o.state != e.state || o.countyOrZone != e.countyOrZone ||
            o.eventType != e.eventType || o.year != e.year ||
            o.displayDate != e.displayDate || o.dateKey != e.dateKey ||
            o.propertyDamage != e.propertyDamage ||
            o.damageKnown != e.damageKnown ||
            o.totalInjuries != e.totalInjuries ||
            o.description != e.description) {
            return false;
        }
    }
    return true;
}

std::string fieldName(SortField f) {
    switch (f) {
        case SortField::Damage: return "Damage";
        case SortField::Injuries: return "Injuries";
        case SortField::Date: return "Date";
    }
    return "?";
}

void testGuideFixture(SortField field, const std::vector<int>& expected) {
    std::vector<StormEvent> before = guideFixture();
    std::vector<StormEvent> work = before;
    mergeSort(work, field);
    std::string name = "guide fixture, " + fieldName(field);
    check(ids(work) == expected, name + ": expected ID order");
    check(work.size() == before.size(), name + ": record count unchanged");
    check(idCounts(work) == idCounts(before), name + ": ID frequencies unchanged");
    check(fieldsStillAttached(before, work), name + ": fields still attached to IDs");
}

void testComparatorBasics() {
    StormEvent knownZero = makeEvent(1, true, 0.0, 0, 202301010000LL);
    StormEvent unknown = makeEvent(2, false, 0.0, 0, 202301010000LL);
    check(comesBefore(knownZero, unknown, SortField::Damage),
          "comparator: known $0 comes before unknown damage");
    check(!comesBefore(unknown, knownZero, SortField::Damage),
          "comparator: unknown damage does not come before known $0");

    StormEvent a = makeEvent(5, true, 100.0, 1, 202301010000LL);
    StormEvent b = a;  // identical, including eventId
    for (SortField f : {SortField::Damage, SortField::Injuries, SortField::Date}) {
        check(!comesBefore(a, b, f) && !comesBefore(b, a, f),
              "comparator: equivalent records false both ways (" + fieldName(f) + ")");
        check(!comesBefore(a, a, f), "comparator: irreflexive (" + fieldName(f) + ")");
    }
}

void testEdgeSizes() {
    std::vector<StormEvent> empty;
    mergeSort(empty, SortField::Damage);
    check(empty.empty(), "empty vector stays empty");

    std::vector<StormEvent> one = {makeEvent(7, true, 50.0, 1, 202305050000LL)};
    mergeSort(one, SortField::Injuries);
    check(one.size() == 1 && one[0].eventId == 7 && one[0].description == "desc-7",
          "one-record vector unchanged");
}

void testSortedAndReversed() {
    std::vector<StormEvent> base;
    for (int i = 0; i < 50; ++i) {
        base.push_back(makeEvent(1000 + i, true, i * 10.0, i, 202301010000LL + i));
    }

    for (SortField f : {SortField::Damage, SortField::Injuries, SortField::Date}) {
        std::vector<StormEvent> sorted = base;
        mergeSort(sorted, f);
        check(isOrdered(sorted, f), "ascending input sorts correctly (" + fieldName(f) + ")");

        std::vector<StormEvent> again = sorted;
        mergeSort(again, f);
        check(ids(again) == ids(sorted), "already-sorted input unchanged (" + fieldName(f) + ")");

        std::vector<StormEvent> reversed(sorted.rbegin(), sorted.rend());
        mergeSort(reversed, f);
        check(ids(reversed) == ids(sorted), "reversed input sorts correctly (" + fieldName(f) + ")");
        check(idCounts(reversed) == idCounts(base), "reversed input keeps every ID (" + fieldName(f) + ")");
    }
}

void testEqualPrimaryKeys() {
    std::vector<StormEvent> v = {
        makeEvent(9, true, 500.0, 3, 202306010000LL),
        makeEvent(3, true, 500.0, 3, 202306010000LL),
        makeEvent(7, true, 500.0, 3, 202306010000LL),
        makeEvent(1, true, 500.0, 3, 202306010000LL),
    };
    for (SortField f : {SortField::Damage, SortField::Injuries, SortField::Date}) {
        std::vector<StormEvent> work = v;
        mergeSort(work, f);
        check(ids(work) == std::vector<int>{1, 3, 7, 9},
              "equal primary keys tie-break by smaller eventId (" + fieldName(f) + ")");
    }
}

void testStability() {
    std::vector<StormEvent> v = {
        makeEvent(50, true, 10.0, 1, 202301010000LL, "A"),
        makeEvent(10, true, 99.0, 9, 202312310000LL, "first"),
        makeEvent(50, true, 10.0, 1, 202301010000LL, "B"),
        makeEvent(50, true, 10.0, 1, 202301010000LL, "C"),
        makeEvent(20, false, 0.0, 0, 202201010000LL, "last"),
        makeEvent(50, true, 10.0, 1, 202301010000LL, "D"),
    };
    for (SortField f : {SortField::Damage, SortField::Injuries, SortField::Date}) {
        std::vector<StormEvent> work = v;
        mergeSort(work, f);
        std::string markers;
        for (const StormEvent& e : work) {
            if (e.eventId == 50) {
                markers += e.description;
            }
        }
        check(markers == "ABCD", "stable for fully equivalent records (" + fieldName(f) + ")");
        check(work.size() == v.size() && idCounts(work) == idCounts(v),
              "no records lost or duplicated with duplicates (" + fieldName(f) + ")");
    }
}

}

int main() {
    testGuideFixture(SortField::Damage, {20, 40, 10, 30});
    testGuideFixture(SortField::Injuries, {30, 10, 40, 20});
    testGuideFixture(SortField::Date, {20, 40, 30, 10});
    testComparatorBasics();
    testEdgeSizes();
    testSortedAndReversed();
    testEqualPrimaryKeys();
    testStability();

    if (failures == 0) {
        std::cout << "\nAll tests passed.\n";
        return 0;
    }
    std::cout << "\n" << failures << " test(s) failed.\n";
    return 1;
}