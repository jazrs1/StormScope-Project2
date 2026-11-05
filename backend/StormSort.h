#pragma once

#include "StormEvent.h"
#include <vector>

//which ranking the user picked.
enum class SortField { Damage, Injuries, Date };

//records equal on every key return false in both directions.
bool comesBefore(const StormEvent& a, const StormEvent& b, SortField field);

//merge sort, william
//sorts the vector it is given; pass a copy, never the master dataset.
void mergeSort(std::vector<StormEvent>& events, SortField field);

//abid's QuickSort.cpp
void quickSort(std::vector<StormEvent>& events, SortField field);