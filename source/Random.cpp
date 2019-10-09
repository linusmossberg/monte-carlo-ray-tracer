#include "Random.h"

thread_local std::mt19937_64 Random::engine;
thread_local unsigned Random::engine_seed;

void Random::seed(unsigned seed)
{
	engine.seed(seed);
	engine_seed = seed;
}

unsigned Random::seed()
{
	return engine_seed;
}

// Generates random numbers in range [min,max[
double Random::range(double v1, double v2)
{
	double min = fmin(v1, v2);
	double max = fmax(v1, v2);

	std::uniform_real_distribution<double> dist(min, std::nextafter(max, min));

	return dist(engine);
}

// Generates random unsigned integers in range [min,max]
size_t Random::uirange(size_t v1, size_t v2)
{
	size_t min = std::min(v1, v2);
	size_t max = std::min(v1, v2);

	std::uniform_int_distribution<size_t> dist(min, max);

	return dist(engine);
}