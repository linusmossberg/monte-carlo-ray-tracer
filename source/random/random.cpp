#include "random.hpp"

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

std::mt19937_64 Random::getEngine()
{
    return engine;
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

size_t Random::weightedUIntSample(const std::vector<double>& weights)
{
    double p = range(0, 1);
    double prev = 0.0;
    size_t i = 0;
    for ( ; i < weights.size() - 1; i++)
    {
        prev += weights[i];
        if (prev > p) return i;
    }
    return i;
}

bool Random::trial(double probability)
{
    return probability > range(0, 1);
}

glm::dvec3 Random::CosWeightedHemiSample()
{
    // Generate uniform sample on unit disk at radius r and angle azimuth
    double u = range(0, 1);
    double r = sqrt(u);
    double azimuth = range(0, C::TWO_PI);

    // Project up to hemisphere.
    // The result is a cosine-weighted hemispherical sample.
    // z = sin(acos(r)) = sqrt(1-r^2) = sqrt(1-sqrt(u)^2) = sqrt(1-u) 
    return glm::dvec3(r * cos(azimuth), r * sin(azimuth), std::sqrt(1 - u));
}

glm::dvec3 Random::UniformHemiSample()
{
    double u = range(0, 1);
    double r = std::sqrt(1.0f - u * u);
    double azimuth = range(0, C::TWO_PI);

    return glm::dvec3(r * cos(azimuth), r * sin(azimuth), u);
}