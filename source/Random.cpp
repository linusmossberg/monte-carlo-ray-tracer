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