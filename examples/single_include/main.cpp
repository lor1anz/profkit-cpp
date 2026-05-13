#define PROFKIT_ENABLE 1
#define PROFKIT_IMPLEMENTATION
#include "profkit.hpp"

#include <cstdint>

void Work() {
  PROF_FUNCTION();

  volatile std::uint64_t value = 0;
  for (std::uint64_t i = 0; i < 10'000'000; ++i) {
    value += i;
  }
}

int main() {
  PROF_SESSION("single include example");

  PROF_USE_MILLISECONDS();

  {
    PROF_SCOPE("work");
    Work();
  }

  return 0;
}