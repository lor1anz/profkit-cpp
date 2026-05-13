#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>

#include "profkit/profiler.hpp"

double Compute() {
	PROF_FUNCTION();

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<double> dis(0.0, 1.0);

	double sum = 0.0;

	{
		PROF_SCOPE("random_loop");

		for (std::uint32_t i = 0; i < 42'000'000; ++i) {
			sum += dis(gen) * (static_cast<double>(i) / INT32_MAX);
		}
	}

	return sum;
}

int main() {
  PROF_SESSION("global");

	const double result = Compute();

	std::cout << std::fixed << std::setprecision(10);
	std::cout << "result: " << result << '\n';

	return 0;
}