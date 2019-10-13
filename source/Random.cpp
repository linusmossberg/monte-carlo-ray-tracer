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

glm::dvec3 Random::CosWeightedHemiSample()
{
    // Generate uniform sample on unit circle at radius r and angle azimuth
    double u = range(0, 1);
    double r = sqrt(u);
    double azimuth = range(0, 2*M_PI);

    // Project up to hemisphere.
    // The result is a cosine-weighted hemispherical sample.
    // z = sin(acos(r)) = sqrt(1-r^2) = sqrt(1-sqrt(u)^2) = sqrt(1-u) 
    return glm::dvec3(r * cos(azimuth), r* sin(azimuth), std::sqrt(1 - u));
}