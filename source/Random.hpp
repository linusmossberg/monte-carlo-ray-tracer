#pragma once

#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/vec3.hpp>

struct Random
{
	Random() {}

	static unsigned seed();

	static void seed(unsigned seed);

	static double range(double v1, double v2);

	static size_t uirange(size_t v1, size_t v2);

	static glm::dvec3 CosWeightedSample();

private:
	// thread_local to create one static random engine per thread.
	thread_local static std::mt19937_64 engine;
	thread_local static unsigned engine_seed;
};
