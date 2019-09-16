#include "Random.h"

std::uniform_real_distribution<double> Random::dist;
std::mt19937_64 Random::engine;

void Random::init(unsigned seed)
{
	dist.param(std::uniform_real_distribution<double>::param_type(0.0, 1.0 - std::numeric_limits<double>::epsilon()));
	engine.seed(seed);
}

// Generates random numbers in range [min,max[
double Random::range(const double& v1, const double& v2)
{
	double min = fmin(v1, v2);
	double max = fmax(v1, v2);

	return min + dist(engine) * (max - min);
}