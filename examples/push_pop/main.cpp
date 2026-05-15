#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>

#include "profkit/profiler.hpp"

double Compute() {

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> dis(0.0, 1.0);

  double sum = 0.0;

  for (std::uint32_t i = 0; i < 1'000'000; ++i) {
    sum += dis(gen) * (static_cast<double>(i) / INT32_MAX);
  }

  return sum;
}

int main() {
  PROF_SESSION("main");
  double result = 0.;

  PROF_PUSH("compute");
  result += Compute();
  PROF_POP();

  PROF_PUSH("compute");
  result += Compute();
  PROF_POP();

  PROF_PUSH("output");
  std::cout << std::fixed << std::setprecision(10);
  std::cout << "result: " << result << '\n';
  PROF_POP();

  return 0;
}
