#include "pixel-operators.hpp"

#include <glm/glm.hpp>

// Tone mapping operator developed by John Hable for Uncharted 2
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
glm::dvec3 filmic(glm::dvec3 in)
{
    const double A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30, W = 11.2;

    auto f = [&](const glm::dvec3& x)
    {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };

    // exposure factor to normalize overall image intensity across tonemapping operators
    in *= 3.0;

    return f(in) / f(glm::dvec3(W));
}

glm::dvec3 gammaCorrect(glm::dvec3 in)
{
    return glm::pow(in, glm::dvec3(1.0 / 2.2));
}

std::vector<uint8_t> truncate(glm::dvec3 in)
{
    in = glm::clamp(in, glm::dvec3(0.0), glm::dvec3(1.0)) * std::nextafter(256.0, 0.0);
    return { (uint8_t)in.b, (uint8_t)in.g, (uint8_t)in.r };
}

glm::dvec3 adjustments(glm::dvec3 in, double midpoint)
{
    //double contrast = 1.1;
    //return contrast * (in - glm::dvec3(midpoint)) + glm::dvec3(midpoint);
    return in;
}

glm::dvec3 reinhard(glm::dvec3 in)
{
    // exposure factor to normalize overall image intensity across tonemapping operators
    in *= 1.5;

    return in / (1.0 + in);
}