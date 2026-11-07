# Sort benchmark notes

## Environment
| Item | Value |
|---|---|
| CPU | Intel(R) Core(TM) Ultra 7 256V |
| RAM | 15.7 GB |
| OS | Microsoft Windows 11 Home |
| Compiler | g++.exe (Rev8, Built by MSYS2 project) 15.2.0 |
| Flags | `-std=c++17 -O2` |
| Git commit | 69b5c1f |
| Data | `data/storms_2023.csv` (75,593 rows) + `data/storms_2024.csv` (69,801 rows) = 145,394 events |
| Shuffle seed | 20261104 |

## Build and run
From the repository root (Windows PowerShell):

```powershell
g++ -std=c++17 -O2 tools/benchmark_sorts.cpp backend/StormLoader.cpp backend/StormFilter.cpp backend/StormSort.cpp backend/MergeSort.cpp backend/QuickSort.cpp -o "$env:TEMP\benchmark_sorts.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\benchmark_sorts.exe" }
```

## Method
- Both CSVs are loaded once and filtered with all filters off (145,394 events) before any timing.
- The combined data is shuffled once with seed 20261104 using a portable Fisher-Yates on `std::mt19937_64`, so the shuffle is the same on any compiler. Sizes 10,000 / 50,000 / 100,000 / 145,394 are prefixes of that shuffle.
- For each size and ranking field (damage, injuries, date), three inputs are prepared outside the timer: shuffled, already sorted (with the verified merge sort), and reverse sorted (`std::reverse`).
- Both algorithms receive identical input: a fresh copy of the same prepared vector is made before every run.
- One untimed warm-up per algorithm and condition, then 5 timed repetitions. The algorithm that runs first alternates each repetition.
- **Timed:** only the sort call (`std::chrono::steady_clock`, milliseconds), including allocations inside the sort, such as merge sort's temporary buffer.
- **Not timed:** loading, filtering, input copying, validation, printing, and CSV output.
- Every timed output is checked afterward: same record count, every adjacent pair ordered by `comesBefore`, and every eventId appears the same number of times as in the input.
- All 360 timed runs (36 conditions x 2 algorithms x 5 repetitions) passed validation. Raw timings are in `sort_benchmarks.csv`.

## Results: median ms (min-max) over 5 runs
| n | field | input order | merge_sort | quick_sort |
|---|---|---|---|---|
| 10000 | damage | shuffled | 16.54 (14.67-17.54) | 21.27 (19.62-21.43) |
| 10000 | damage | sorted | 12.92 (10.67-13.10) | 14.94 (13.60-15.71) |
| 10000 | damage | reversed | 9.40 (9.22-10.43) | 11.91 (11.38-12.20) |
| 10000 | injuries | shuffled | 18.82 (18.56-19.52) | 23.94 (23.20-29.77) |
| 10000 | injuries | sorted | 14.77 (14.51-15.55) | 17.55 (17.29-17.88) |
| 10000 | injuries | reversed | 14.58 (13.96-15.15) | 17.54 (17.05-18.09) |
| 10000 | date | shuffled | 18.65 (18.32-19.01) | 24.91 (23.56-26.11) |
| 10000 | date | sorted | 14.64 (13.18-16.25) | 17.23 (16.23-19.85) |
| 10000 | date | reversed | 13.16 (12.02-13.90) | 15.60 (15.01-16.51) |
| 50000 | damage | shuffled | 109.17 (106.61-112.08) | 150.65 (148.89-176.63) |
| 50000 | damage | sorted | 77.58 (77.25-77.85) | 95.11 (92.47-95.51) |
| 50000 | damage | reversed | 75.82 (73.14-77.48) | 95.17 (91.71-96.31) |
| 50000 | injuries | shuffled | 105.41 (95.42-119.24) | 154.41 (139.95-166.83) |
| 50000 | injuries | sorted | 83.00 (73.86-95.57) | 93.87 (55.81-97.14) |
| 50000 | injuries | reversed | 46.38 (45.75-47.48) | 54.45 (54.22-60.74) |
| 50000 | date | shuffled | 68.13 (66.25-70.23) | 92.28 (87.72-94.02) |
| 50000 | date | sorted | 47.19 (46.25-50.59) | 58.92 (54.25-59.62) |
| 50000 | date | reversed | 46.82 (45.49-69.41) | 58.02 (54.53-79.71) |
| 100000 | damage | shuffled | 137.98 (130.41-147.72) | 180.02 (172.60-185.53) |
| 100000 | damage | sorted | 96.14 (92.11-98.11) | 105.07 (103.65-106.97) |
| 100000 | damage | reversed | 90.93 (90.14-99.21) | 104.77 (103.15-110.67) |
| 100000 | injuries | shuffled | 126.46 (124.62-130.87) | 173.21 (172.58-174.33) |
| 100000 | injuries | sorted | 94.06 (92.31-94.45) | 106.01 (104.70-107.38) |
| 100000 | injuries | reversed | 93.51 (92.03-95.78) | 106.32 (105.77-107.59) |
| 100000 | date | shuffled | 128.17 (126.75-128.41) | 168.78 (168.11-173.24) |
| 100000 | date | sorted | 97.08 (95.85-98.55) | 110.16 (108.88-111.95) |
| 100000 | date | reversed | 95.67 (95.01-96.48) | 111.56 (108.74-111.80) |
| 145394 | damage | shuffled | 194.00 (192.38-198.12) | 256.42 (253.97-265.68) |
| 145394 | damage | sorted | 134.49 (131.59-134.65) | 153.78 (150.63-160.09) |
| 145394 | damage | reversed | 132.48 (131.46-134.76) | 155.52 (152.90-156.60) |
| 145394 | injuries | shuffled | 190.70 (186.66-209.29) | 247.75 (244.92-284.25) |
| 145394 | injuries | sorted | 140.32 (138.38-163.73) | 157.93 (154.92-163.76) |
| 145394 | injuries | reversed | 136.83 (134.49-144.60) | 155.01 (154.23-159.48) |
| 145394 | date | shuffled | 190.33 (187.37-191.68) | 257.30 (253.60-261.49) |
| 145394 | date | sorted | 142.55 (139.76-147.23) | 160.95 (160.56-161.39) |
| 145394 | date | reversed | 140.22 (139.40-203.66) | 160.83 (160.33-161.59) |

### Full dataset (n = 145,394), median ms
| Field | Input | Merge sort | Quicksort | Quick / merge |
|---|---|---|---|---|
| Damage | shuffled | 194.00 | 256.42 | 1.32x |
| Damage | sorted | 134.49 | 153.78 | 1.14x |
| Damage | reversed | 132.48 | 155.52 | 1.17x |
| Injuries | shuffled | 190.70 | 247.75 | 1.30x |
| Injuries | sorted | 140.32 | 157.93 | 1.13x |
| Injuries | reversed | 136.83 | 155.01 | 1.13x |
| Date | shuffled | 190.33 | 257.30 | 1.35x |
| Date | sorted | 142.55 | 160.95 | 1.13x |
| Date | reversed | 140.22 | 160.83 | 1.15x |

## Observations
- **Merge sort had the lower median in all 36 conditions.** On shuffled input, quicksort took about 1.3-1.45x as long; on sorted and reversed input, about 1.1-1.25x. This holds for these two implementations on this machine, not for the algorithms in general. Likely contributors: this quicksort copies the pivot record and swaps whole `StormEvent` records (each holding several strings), while the merge sort moves records into one preallocated buffer.
- **Sorted and reversed inputs were faster than shuffled for both algorithms** (about 25-30% faster on the full dataset). Ordered data makes comparisons more predictable for the CPU and memory access more regular. Neither algorithm skips work on sorted input: merge sort still performs every merge level.
- **Quicksort's worst case:** These tested inputs did not demonstrate quadratic worst-case behavior. Middle-element pivot selection does not guarantee balanced partitions, so worst-case time remains O(n^2). Three-way partitioning groups records that compare as equal; records with the same ranking value can still differ because eventId breaks ties.
- **Timing noise:** some 50,000-row conditions are out of line with their neighbours (for example, the date and reversed-injuries rows at n = 50,000 are faster than the damage rows), and a few max values are well above their medians. This is consistent with CPU frequency changes or background activity on a laptop. The merge-vs-quick comparison within each condition is more reliable than the growth between sizes, because the two algorithms ran alternately on identical input.
- **Hardware note:** the Core Ultra 7 256V is a power-managed laptop CPU whose clock speed varies with temperature and power settings, which likely explains the uneven 50,000-row timings.

## Complexity
- **Merge sort:** O(n log n) time in the best, average, and worst case; O(n) auxiliary records for the temporary buffer plus O(log n) recursion depth. Stable.
- **Quicksort:** O(n log n) average time, but O(n^2) worst case when pivots repeatedly split the range badly. This implementation recurses into the smaller side and loops on the larger, which keeps stack depth at O(log n). Not stable.
- These bounds count record comparisons and moves. Copying the variable-length strings inside each record adds real cost that the bounds do not show. The auxiliary-memory figures are theoretical; memory use was not measured.