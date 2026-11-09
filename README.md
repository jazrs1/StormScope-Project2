# StormScope

StormScope is a website for exploring NOAA storm events from 2023 and 2024. Users can filter events by state, year, and event type, then rank them by property damage, injuries, or date.

We implemented merge sort and quicksort from scratch in C++ and compare their performance on the same records. Our dataset contains 145,394 event records.

## Team

**Storm Troopers — COP3530 Project 2**

- John Azer — [jazrs1](https://github.com/jazrs1)
- William Lin — [aSquidE](https://github.com/aSquidE)
- Abid Panahie — [SaucyChickenHZ](https://github.com/SaucyChickenHZ)

**Video demo:** https://www.youtube.com/watch?v=igL0JHEg54o

## Features

- Filter events by state, year, and event type.
- Rank events by highest property damage, most injuries, or newest date.
- Choose quicksort, merge sort, or compare both.
- View sorting times and check whether both algorithms produce the same order.
- Browse results in pages of 50 events.
- Open event details, including the NOAA event narrative.

## Requirements

- Git
- A C++17 compiler:
  - Windows: g++ / MinGW
  - macOS: clang++
- Python 3 with support for virtual environments and pip
- The NOAA CSV files described below

The website runs locally using Flask. Opening `web/index.html` directly will not run the application.

## Setup

### 1. Clone the repository

```bash
git clone https://github.com/jazrs1/StormScope-Project2.git
cd StormScope-Project2
```

Run all remaining commands from this repository folder.

### 2. Download the data

Visit the [NOAA Storm Events CSV downloads](https://www.ncei.noaa.gov/pub/data/swdi/stormevents/csvfiles/).

Download the `StormEvents_details` files for **2023** and **2024**. Use the details files, not the locations or fatalities files.

Extract each `.csv.gz` file. Create a `data` folder in the repository and rename the extracted CSV files as follows:

```text
data/storms_2023.csv
data/storms_2024.csv
```

Our tested files contain:

| Year | Records |
| --- | ---: |
| 2023 | 75,593 |
| 2024 | 69,801 |
| Total | 145,394 |

NOAA may revise its files, so newer downloads can have different record counts. Our benchmark results use the dataset containing 145,394 records.

### 3. Build the C++ driver

**Windows PowerShell**

```powershell
New-Item -ItemType Directory -Force build
g++ -std=c++17 -Wall -Wextra -O2 -static backend/storm_cli.cpp backend/StormLoader.cpp backend/StormFilter.cpp backend/StormSort.cpp backend/MergeSort.cpp backend/QuickSort.cpp -o build/storm_cli.exe
```

**macOS**

```bash
mkdir -p build
clang++ -std=c++17 -Wall -Wextra -O2 backend/storm_cli.cpp backend/StormLoader.cpp backend/StormFilter.cpp backend/StormSort.cpp backend/MergeSort.cpp backend/QuickSort.cpp -o build/storm_cli
```

Rebuild the driver after changing the C++ code. Make sure compilation succeeds before continuing.

### 4. Install the Python dependencies

**Windows PowerShell**

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r server/requirements.txt
```

**macOS**

```bash
python3 -m venv .venv
.venv/bin/python -m pip install -r server/requirements.txt
```

### 5. Start the website

**Windows PowerShell**

```powershell
.\.venv\Scripts\python.exe -m flask --app server/app run --port 5000
```

**macOS**

```bash
.venv/bin/python -m flask --app server/app run --port 5000
```

Open **http://127.0.0.1:5000** in your browser.

Keep the terminal running while using the website. Press **Ctrl+C** in that terminal to stop the server.

## Using StormScope

1. Select a state, year, and event type, or leave them set to All.
2. Choose property damage, injuries, or date under Rank by.
3. Select Quicksort, Merge sort, or Compare both.
4. Click Analyze.
5. Use Previous and Next to browse results, or Details to read about an event.

With our tested dataset, filtering for **FLORIDA**, **2024**, and **Flood** returns **73 events**.

The displayed time measures only the sorting call. It excludes CSV loading, filtering, process startup, and sending results to the browser. Small inputs may sort too quickly for the timer to measure reliably.

## Algorithms and Ordering

Both sorting algorithms are implemented from scratch and use the same comparison function.

- **Property damage:** highest amount first; unknown damage comes last. A reported $0 is treated as known.
- **Injuries:** highest total first, combining direct and indirect injuries.
- **Date:** newest date key first.
- **Ties:** smaller event ID first.

Dates use the local times recorded in the source files; they are not converted to a shared UTC timezone.

Merge sort recursively sorts and merges two halves using a reusable temporary vector. Quicksort uses a middle-element pivot and three-way partitioning. It recurses on the smaller partition and loops over the larger partition to limit recursion depth.

| Algorithm | Average time | Worst-case time | Auxiliary space |
| --- | --- | --- | --- |
| Merge sort | O(n log n) | O(n log n) | O(n) |
| Quicksort | O(n log n) | O(n²) | O(log n) recursion stack |

Here, `n` is the number of filtered records being sorted. These bounds treat record operations and comparisons as constant cost; string copying adds work.

## Tests

Run these commands from the repository root in **Windows PowerShell**.

### CSV loader validation

```powershell
g++ -std=c++17 -Wall -Wextra tools/test_loader.cpp backend/StormLoader.cpp -o "$env:TEMP\test_loader.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\test_loader.exe" }
```

### Merge sort

```powershell
g++ -std=c++17 -Wall -Wextra tools/test_merge_sort.cpp backend/MergeSort.cpp backend/StormSort.cpp -o "$env:TEMP\test_merge_sort.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\test_merge_sort.exe" }
```

### Quicksort

```powershell
g++ -std=c++17 -Wall -Wextra tools/test_quick_sort.cpp backend/QuickSort.cpp backend/MergeSort.cpp backend/StormSort.cpp -o "$env:TEMP\test_quick_sort.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\test_quick_sort.exe" }
```

The quicksort test also links merge sort because it compares the algorithms' outputs.

### Real-data loader and filter preview

```powershell
g++ -std=c++17 tools/preview_event.cpp backend/StormLoader.cpp backend/StormFilter.cpp -o "$env:TEMP\preview_event.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\preview_event.exe" }
```

On macOS, use `clang++` with the same source files, write the executable to `/tmp/<name>`, and run it with `&& /tmp/<name>` after successful compilation.

Compile each test or preview separately. Each file in `tools/` has its own `main` function.

## Benchmarks

The benchmark compares both algorithms using:

- 10,000, 50,000, 100,000, and 145,394 records
- Damage, injuries, and date rankings
- Shuffled, sorted, and reversed inputs
- One warm-up and five timed repetitions per condition

Each algorithm receives an identical copy of the input. The benchmark alternates which algorithm runs first and checks ordering, record count, and event ID frequencies after sorting.

Run in **Windows PowerShell**:

```powershell
New-Item -ItemType Directory -Force results
g++ -std=c++17 -O2 tools/benchmark_sorts.cpp backend/StormLoader.cpp backend/StormFilter.cpp backend/StormSort.cpp backend/MergeSort.cpp backend/QuickSort.cpp -o "$env:TEMP\benchmark_sorts.exe"
if ($LASTEXITCODE -eq 0) { & "$env:TEMP\benchmark_sorts.exe" }
```

- [Recorded benchmark results](results/sort_benchmarks.csv)
- [Benchmark method, machine specifications, and analysis](results/benchmark_notes.md)

Rerunning the benchmark overwrites `results/sort_benchmarks.csv`. Update the notes if you commit results from a different machine or run.

## Project Structure

| Folder | Contents |
| --- | --- |
| `backend/` | C++ event structure, CSV loader, filters, sorting algorithms, and JSON driver |
| `server/` | Flask server and Python requirements |
| `web/` | HTML, CSS, and JavaScript interface |
| `tools/` | Tests, previews, and benchmark program |
| `results/` | Recorded benchmark results and analysis |
| `data/` | Locally downloaded NOAA CSV files |
| `build/` | Locally compiled C++ driver |

## Troubleshooting

**C++ driver not found**

Build the driver using the commands above. Rebuild it after changing any C++ source files.

**Could not open a CSV file**

Check that both extracted CSV files are in `data/` with the exact filenames `storms_2023.csv` and `storms_2024.csv`.

**Could not reach the StormScope server**

Check that Flask is still running in the terminal. Restart it if needed, then reload the browser.

**Port 5000 is already in use**

Change `--port 5000` to `--port 5050` in the startup command and open http://127.0.0.1:5050.

**Website changes are not showing**

Hard-refresh the browser to reload the JavaScript and CSS.

**Searches or page changes take a moment**

Each request starts the C++ driver and reloads, filters, and sorts the data. The displayed sorting time does not include all of this work.

## Data and Dependencies

- [NOAA Storm Events Database](https://www.ncei.noaa.gov/stormevents/)
- [NOAA CSV downloads](https://www.ncei.noaa.gov/pub/data/swdi/stormevents/csvfiles/)
- Flask: used as the local web server; dependencies are listed in `server/requirements.txt`.
- C++ standard library: used for supporting containers, file handling, and timing.

Both event-sorting algorithms were implemented by our team without using library sorting functions.
