#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "profkit/profiler.hpp"

double Compute(std::uint32_t iterations) {
  PROF_FUNCTION();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(0.0, 1.0);

  double sum = 0.0;

  {
    PROF_SCOPE("random_loop");

    for (std::uint32_t i = 0; i < iterations; ++i) {
      sum += dis(gen) * (static_cast<double>(i) / INT32_MAX);
    }
  }

  return sum;
}

int main() {
  PROF_SESSION("global");

  constexpr std::uint32_t kThreads = 4;
  constexpr std::uint32_t kIterations = 10'000'000;

  std::vector<std::thread> threads;
  std::vector<double> results(kThreads, 0.0);

  {
    PROF_SCOPE("launch_threads");

    for (std::uint32_t thread = 0; thread < kThreads; ++thread) {
      threads.emplace_back([thread, &results] {
        PROF_SCOPE("worker_thread");
        results[thread] = Compute(kIterations);
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }
  }

  double total = 0.0;
  for (double value : results) {
    total += value;
  }

  std::cout << std::fixed << std::setprecision(10);
  std::cout << "result: " << total << '\n';

  return 0;
}