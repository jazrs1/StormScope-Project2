#include "StormSort.h"

#include <cstddef>
#include <utility>

namespace {

//merge the two sorted halves of events
void merge(std::vector<StormEvent>& events, std::vector<StormEvent>& temp,
           std::size_t left, std::size_t mid, std::size_t right,
           SortField field) {
    std::size_t i = left; 
    std::size_t j = mid;  
    std::size_t k = left;  

    while (i < mid && j < right) {
        if (!comesBefore(events[j], events[i], field)) {
            temp[k] = std::move(events[i]);
            ++i;
        } else {
            temp[k] = std::move(events[j]);
            ++j;
        }
        ++k;
    }

    while (i < mid) {
        temp[k] = std::move(events[i]);
        ++i;
        ++k;
    }
    while (j < right) {
        temp[k] = std::move(events[j]);
        ++j;
        ++k;
    }

    for (std::size_t p = left; p < right; ++p) {
        events[p] = std::move(temp[p]);
    }
}

//sort the half open range
void mergeSortRange(std::vector<StormEvent>& events, std::vector<StormEvent>& temp,
                    std::size_t left, std::size_t right, SortField field) {
    if (right - left <= 1) {
        return;
    }

    std::size_t mid = left + (right - left) / 2;
    mergeSortRange(events, temp, left, mid, field);
    mergeSortRange(events, temp, mid, right, field);
    merge(events, temp, left, mid, right, field);
}

} 

void mergeSort(std::vector<StormEvent>& events, SortField field) {
    if (events.size() < 2) {
        return;
    }
    std::vector<StormEvent> temp(events.size());
    mergeSortRange(events, temp, 0, events.size(), field);
}