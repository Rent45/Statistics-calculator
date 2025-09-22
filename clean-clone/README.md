# Statistics-calculator

A C++ console programme (ALGLIB-based) for practising basic statistical tasks: confidence intervals, hypothesis testing and χ²-tests.  
Designed for learning: the programme can check user answers, show step-by-step solutions and generate random problems.

---

## Features

- Confidence intervals for means and proportions (z and t).
- Hypothesis testing for proportions.
- Sample size calculation for a required margin of error.
- χ² tests for independence and goodness-of-fit.
- Menu-driven console UI (arrow keys + Enter).
- Step-by-step solutions available on demand.
- Random or manual parameter/problem input.
- Uses ALGLIB for special functions and inverse distributions.

---

## Status & platform

> **Status:** Educational / student project.  
> **Platform:** Windows (code uses `<conio.h>`, `windows.h` and Windows console APIs like `SetConsoleTextAttribute`, `GetStdHandle`).  
Porting to Linux / macOS will require removing or replacing Windows-specific console code.

---

## Repository layout (recommended)
Final/ ← repo root
├─ main.cpp
├─ CMakeLists.txt
├─ Types of problems.txt
├─ Types inside code.txt
├─ Solutions inside code.txt
├─ Special Solutions.txt
├─ Author.txt
└─ external_alglib/ ← ALGLIB sources & headers

---

## Prerequisites

- C++ compiler with C++20 support (MinGW / MSVC recommended on Windows).
- CMake (>= 3.16 recommended).
- CLion (optional) or any IDE that uses CMake.
- ALGLIB sources (either inside `external_alglib/` in the repo, or provide the path via CMake variable `ALGLIB_ROOT`).

---

## Build & run
# Using CLion

Open the project directory in CLion.
CLion will load the CMakeLists.txt automatically. If not, Reload CMake project.
Build the Final target and run it from CLion (use the CMake target run configuration).

**Important:** do not run the raw main.cpp single-file configuration — that bypasses CMake (and ALGLIB sources / include paths), causing specialfunctions.h not found errors.

ALGLIB link used:  https://www.alglib.net/
