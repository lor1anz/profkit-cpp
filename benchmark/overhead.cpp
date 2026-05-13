#include <chrono>
#include <cstdint>
#include <iostream>

#include "profkit/profiler.hpp"

namespace {

volatile std::uint64_t sink = 0;

std::uint64_t NowNs() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

void EmptyLoop(std::uint64_t iterations) {
  for (std::uint64_t i = 0; i < iterations; ++i) {
    sink += i;
  }
}

void ProfiledLoop(std::uint64_t iterations) {
  for (std::uint64_t i = 0; i < iterations; ++i) {
    PROF_SCOPE("bench.scope");
    sink += i;
  }
}

void RunBenchmark(std::uint64_t iterations) {
  const auto empty_start = NowNs();
  EmptyLoop(iterations);
  const auto empty_end = NowNs();

  PROF_RESET();

  const auto profiled_start = NowNs();
  ProfiledLoop(iterations);
  const auto profiled_end = NowNs();

  const auto empty_ns = empty_end - empty_start;
  const auto profiled_ns = profiled_end - profiled_start;
  const auto overhead_ns = profiled_ns - empty_ns;

  std::cout << "iterations:        " << iterations << '\n';
  std::cout << "empty loop ms:     " << static_cast<double>(empty_ns) / 1'000'000.0 << '\n';
  std::cout << "profiled loop ms:  " << static_cast<double>(profiled_ns) / 1'000'000.0 << '\n';
  std::cout << "overhead ms:       " << static_cast<double>(overhead_ns) / 1'000'000.0 << '\n';
  std::cout << "overhead ns/call:  "
            << static_cast<double>(overhead_ns) / static_cast<double>(iterations) << '\n';

  PROF_DUMP();
}

}  // namespace

int main() {
  constexpr std::uint64_t kIterations = 10'000'000;
  RunBenchmark(kIterations);
}