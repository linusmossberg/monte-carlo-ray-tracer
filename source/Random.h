#pragma once

#include <random>

struct Random
{
	Random() {}

	static void init(unsigned seed);

	static double range(const double& v1, const double& v2);

	static std::mt19937_64 engine;
	static std::uniform_real_distribution<double> dist;
};
