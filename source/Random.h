#pragma once

#include <random>

struct Random
{
	Random() {}

	static unsigned seed();

	static void seed(unsigned seed);

	static double range(double v1, double v2);

	static size_t uirange(size_t v1, size_t v2);

private:
	// thread_local to create one static random engine per thread.
	thread_local static std::mt19937_64 engine;
	thread_local static unsigned engine_seed;
};
