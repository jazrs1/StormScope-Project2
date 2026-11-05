#include "StormSort.h"

#include <cstddef>
#include <utility>

namespace {


void partition3(std::vector<StormEvent>& events, std::size_t lo, std::size_t hi,
                SortField field, std::size_t& ltOut, std::size_t& gtOut) {

    const StormEvent pivot = events[lo + (hi - lo) / 2];

    std::size_t lt = lo;
    std::size_t i = lo;
    std::size_t gt = hi;  

    while (i < gt) {
        if (comesBefore(events[i], pivot, field)) {
            std::swap(events[lt], events[i]);
            ++lt;
            ++i;
        } else if (comesBefore(pivot, events[i], field)) {
            --gt;
            std::swap(events[i], events[gt]);

        } else {
            ++i;
        }
    }

    ltOut = lt;
    gtOut = gt;
}


void quickSortRange(std::vector<StormEvent>& events, std::size_t lo, std::size_t hi,
                    SortField field) {
    while (hi - lo > 1) {
        std::size_t lt = 0;
        std::size_t gt = 0;
        partition3(events, lo, hi, field, lt, gt);

        const std::size_t leftSize = lt - lo;
        const std::size_t rightSize = hi - gt;

        if (leftSize < rightSize) {
            quickSortRange(events, lo, lt, field);
            lo = gt;   
        } else {
            quickSortRange(events, gt, hi, field);
            hi = lt;   
        }
    }
}

}  


void quickSort(std::vector<StormEvent>& events, SortField field) {
    quickSortRange(events, 0, events.size(), field);
}