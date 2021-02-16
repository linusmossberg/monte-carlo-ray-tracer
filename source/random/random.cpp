#include "random.hpp"

#include <algorithm>

#include "../common/constants.hpp"

double Random::unit()
{
    static thread_local std::uniform_real_distribution<double> unit_distribution(0.0, std::nextafter(1.0, 0.0));
    return unit_distribution(engine);
}

double Random::angle()
{
    static thread_local std::uniform_real_distribution<double> angle_distribution(0.0, std::nextafter(C::TWO_PI, 0.0));
    return angle_distribution(engine);
}

size_t Random::weightedIdxSample(const std::vector<double>& w)
{
    double p = unit();
    return std::find_if(w.begin(), w.end() - 1, [p](const auto &v) { return v > p; }) - w.begin();
}

bool Random::trial(double probability)
{
    return probability > Random::unit();
}

glm::dvec2 Random::uniformDiskSample()
{
    double azimuth = Random::angle();
    return glm::dvec2(std::cos(azimuth), std::sin(azimuth)) * std::sqrt(unit());
}

glm::dvec3 Random::cosWeightedHemiSample()
{
    // Generate uniform sample on unit disk at radius r and angle azimuth
    double u = unit();
    double r = std::sqrt(u);
    double azimuth = Random::angle();

    // Project up to hemisphere.
    // z = sin(acos(r)) = sqrt(1-r^2) = sqrt(1-sqrt(u)^2) = sqrt(1-u) 
    return glm::dvec3(r * std::cos(azimuth), r * std::sin(azimuth), std::sqrt(1 - u));
}

glm::dvec3 Random::uniformHemiSample()
{
    double u = unit();
    double r = std::sqrt(1.0 - u * u);
    double azimuth = Random::angle();

    return glm::dvec3(r * std::cos(azimuth), r * std::sin(azimuth), u);
}
