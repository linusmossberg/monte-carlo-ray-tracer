#include "pixel-operators.hpp"

#include <glm/glm.hpp>

// Tone mapping operator developed by John Hable for Uncharted 2
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
glm::dvec3 filmicHable(const glm::dvec3 &in)
{
    const double A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30, W = 11.2;
    //const double A = 0.22, B = 0.30, C = 0.10, D = 0.20, E = 0.01, F = 0.30, W = 11.2;

    auto f = [&](const glm::dvec3& x)
    {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };

    return f(in) / f(glm::dvec3(W));
}

// From https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
glm::dvec3 filmicACES(const glm::dvec3 &in)
{
    const double A = 2.51, B = 0.03, C = 2.43, D = 0.59, E = 0.14;
    return glm::clamp((in * (A * in + B)) / (in * (C * in + D) + E), 0.0, 1.0);
}

glm::dvec3 gammaCorrect(const glm::dvec3 &in)
{
    return glm::pow(in, glm::dvec3(1.0 / 2.2));
}

std::vector<uint8_t> truncate(const glm::dvec3 &in)
{
    glm::dvec3 c = glm::clamp(in, glm::dvec3(0.0), glm::dvec3(1.0)) * std::nextafter(256.0, 0.0);
    return { (uint8_t)c.b, (uint8_t)c.g, (uint8_t)c.r };
}