#include "Random.hpp"

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
double Random::range(double min, double max)
{
	return std::uniform_real_distribution<double>(min, std::nextafter(max, min))(engine);
}

// Generates random unsigned integers in range [min,max]
size_t Random::uirange(size_t min, size_t max)
{
	return std::uniform_int_distribution<size_t>(min, max)(engine);
}

glm::dvec3 Random::CosWeightedSample()
{
	// Generate uniform sample on unit circle at radius r and angle azimuth
	double r = sqrt(range(0,1));
	double azimuth = range(0.0, 2.0*M_PI);

	// Project up to hemisphere.
	// The result is a cosine-weighted hemispherical sample.
	glm::dvec3 local_dir(r * cos(azimuth), r * sin(azimuth), sin(acos(r)));

	return local_dir;
}