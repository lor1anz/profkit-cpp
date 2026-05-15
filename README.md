# profkit-cpp

A small C++ profiling library for scoped instrumentation.

`profkit-cpp` is designed to be:

* simple to integrate
* lightweight
* usable in multithreaded applications
* easy to disable completely at compile time
* convenient for local profiling of CPU code

The library provides:

* RAII scoped profiling
* function profiling macros
* profiling sessions
* multithreaded statistics aggregation
* configurable time units
* single-header distribution
* low-overhead instrumentation

---

# Features

* `PROF_SCOPE("...")`
* `PROF_FUNCTION()`
* `PROF_SESSION("...")`
* `PROF_DUMP()`
* `PROF_RESET()`
* `PROF_DUMP_AND_RESET()`
* `PROF_USE_SECONDS()`
* `PROF_USE_MILLISECONDS()`

Collected statistics:

* wall time
* total CPU time across threads
* average call time
* minimum call time
* maximum call time
* maximum accumulated time per thread
* number of calls

---

# Example

```cpp
#include "profkit/profiler.hpp"

void Work() {
  PROF_FUNCTION();

  {
    PROF_SCOPE("heavy_loop");

    volatile int value = 0;
    for (int i = 0; i < 100000000; ++i) {
      value += i;
    }
  }
}

int main() {
  PROF_SESSION("main");

  PROF_USE_SECONDS();

  Work();

  return 0;
}
```

Example output:

```text
Profiler report
=================================================================================================================================================

scope                                             calls      wall s     cpu total s         avg s         min s         max s      max thread s
-------------------------------------------------------------------------------------------------------------------------------------------------
main                                                   1        1.425           1.425         1.425         1.425         1.425              1.425
void Work()                                            1        1.423           1.423         1.423         1.423         1.423              1.423
heavy_loop                                             1        1.422           1.422         1.422         1.422         1.422              1.422
```

---

# Building

## Requirements

* CMake 3.24+
* C++20 compiler

Tested with:

* GCC
* Clang

---

## Configure

```bash
cmake --preset production
```

---

## Build

```bash
cmake --build --preset production
```

---

# CMake Integration

## add_subdirectory

```cmake
add_subdirectory(third_party/profkit-cpp)

add_executable(my_app main.cpp)

target_link_libraries(my_app PRIVATE profkit)
```

---

## FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
  profkit
  GIT_REPOSITORY <repository-url>
  GIT_TAG master
)

FetchContent_MakeAvailable(profkit)

add_executable(my_app main.cpp)

target_link_libraries(my_app PRIVATE profkit)
```

---

# Profiling Macros

## PROF_SCOPE

Profiles a local scope.

```cpp
{
  PROF_SCOPE("simulation");
  RunSimulation();
}
```

---

## PROF_FUNCTION

Profiles the current function.

```cpp
void Compute() {
  PROF_FUNCTION();
}
```

The function name is automatically extracted using:

* `__PRETTY_FUNCTION__` on GCC/Clang
* `__FUNCSIG__` on MSVC

---

## PROF_SESSION

Creates a profiling session.

When the session object leaves scope:

* the profiling scope is closed
* the report is automatically printed

```cpp
int main() {
  PROF_SESSION("main");

  Work();

  return 0;
}
```

---

## PROF_DUMP

Prints the current profiling report.

```cpp
PROF_DUMP();
```

---

## PROF_RESET

Resets accumulated statistics.

```cpp
PROF_RESET();
```

---

## PROF_DUMP_AND_RESET

Prints the report and resets collected statistics.

```cpp
PROF_DUMP_AND_RESET();
```

---

# Time Units

## PROF_USE_SECONDS

```cpp
PROF_USE_SECONDS();
```

---

## PROF_USE_MILLISECONDS

```cpp
PROF_USE_MILLISECONDS();
```

Milliseconds are used by default.

---

# Multithreading

`profkit-cpp` supports multithreaded profiling.

Each thread stores profiling statistics in thread-local storage.

The final report aggregates statistics across all threads.

The following values are reported:

* wall time
* accumulated CPU time
* maximum accumulated time on a single thread

Example:

```cpp
void Worker() {
  PROF_SCOPE("worker");
  Compute();
}

int main() {
  PROF_SESSION("main");

  std::thread t1(Worker);
  std::thread t2(Worker);

  t1.join();
  t2.join();
}
```

---

# Disabling Profiling

Profiling can be completely disabled at compile time.

```cmake
-DPROFKIT_ENABLE=OFF
```

When disabled:

* profiling macros become no-op
* instrumentation overhead disappears after optimization

---

# Single Header Distribution

A generated single-header version is available.

Generate it:

```bash
python3 make_single_include.py
```

This generates:

```text
single_include/profkit.hpp
```

---

## Usage

In exactly one translation unit:

```cpp
#define PROFKIT_ENABLE 1
#define PROFKIT_IMPLEMENTATION
#include "profkit.hpp"
```

In all other translation units:

```cpp
#define PROFKIT_ENABLE 1
#include "profkit.hpp"
```

---

# Overhead Benchmark

The project contains a simple instrumentation overhead benchmark.

Build and run:

```bash
cmake --preset release
cmake --build --preset release
./build/release/benchmark/profkit_overhead
```

---

The benchmark measures:

* empty loop execution time
* profiled loop execution time
* approximate instrumentation overhead
* average overhead per scope call

---

# License

MIT License
