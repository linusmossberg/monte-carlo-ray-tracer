#include "pixel-operators.hpp"

#include <glm/glm.hpp>

// Tone mapping operator developed by John Hable for Uncharted 2
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
glm::dvec3 filmicHable(const glm::dvec3 &in)
{
    constexpr double A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30, W = 11.2;
    //constexpr double A = 0.22, B = 0.30, C = 0.10, D = 0.20, E = 0.01, F = 0.30, W = 11.2;

    auto f = [&](const glm::dvec3& x)
    {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };

    return f(in) / f(glm::dvec3(W));
}

glm::dvec3 filmicACES(const glm::dvec3 &in)
{
    constexpr glm::dmat3 ACESInputMat(glm::dvec3(0.59719, 0.07600, 0.02840),
                                      glm::dvec3(0.35458, 0.90834, 0.13383),
                                      glm::dvec3(0.04823, 0.01566, 0.83777));

    constexpr glm::dmat3 ACESOutputMat(glm::dvec3(1.60475, -0.10208, -0.00327),
                                       glm::dvec3(-0.53108, 1.10813, -0.07276),
                                       glm::dvec3(-0.07367, -0.00605, 1.07602));

    auto RRTAndODTFit = [](const glm::dvec3 &v)
    {
        glm::dvec3 a = v * (v + 0.0245786) - 0.000090537;
        glm::dvec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
        return a / b;
    };

    glm::dvec3 color = ACESOutputMat * RRTAndODTFit(ACESInputMat * in);

    return glm::clamp(color, 0.0, 1.0);
}

glm::dvec3 linear(const glm::dvec3 &in)
{
    return in;
}

std::vector<uint8_t> truncate(const glm::dvec3 &in)
{
    glm::dvec3 c = glm::clamp(in, glm::dvec3(0.0), glm::dvec3(1.0)) * std::nextafter(256.0, 0.0);
    return { (uint8_t)c.b, (uint8_t)c.g, (uint8_t)c.r };
}