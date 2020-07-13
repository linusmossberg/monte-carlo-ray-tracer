#pragma once

#include <set>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "illuminant.hpp"

// Using CIE 1931 2°
namespace CIE
{
    // wavelength to CIE XYZ color matching functions value
    glm::dvec3 CMF(double wavelength);

    // Chromaticity coordinates xy and luminance Y to XYZ
    constexpr glm::dvec3 XYZ(const glm::dvec2 &xy, double Y)
    {
        double N = Y / xy.y;
        return { N * xy.x, Y, N * (1.0 - xy.x - xy.y) };
    }

    struct SpectralValue
    {
        SpectralValue(double wavelength, double value) : wavelength(wavelength), value(value) { }
        bool operator< (const SpectralValue& rhs) const { return wavelength < rhs.wavelength; };
        double wavelength;
        double value;
    };

    // Spectral distribution to XYZ
    glm::dvec3 XYZ(const std::set<SpectralValue> &distribution);

    // Spectral reflectance distribution to sRGB
    glm::dvec3 RGB(const std::set<SpectralValue> &distribution);

    // sRGB to XYZ_D65
    constexpr glm::dvec3 XYZ(const glm::dvec3 &RGB)
    {
        return
        {
            0.41239080 * RGB.r + 0.35758434 * RGB.g + 0.18048079 * RGB.b,
            0.21263901 * RGB.r + 0.71516868 * RGB.g + 0.07219232 * RGB.b,
            0.01933082 * RGB.r + 0.11919478 * RGB.g + 0.95053215 * RGB.b
        };
    }

    // XYZ_D65 to sRGB
    constexpr glm::dvec3 RGB(const glm::dvec3 &XYZ)
    {
        return
        {
             3.24096994 * XYZ.x - 1.53738318 * XYZ.y - 0.49861076 * XYZ.z,
            -0.96924364 * XYZ.x + 1.87596750 * XYZ.y + 0.04155506 * XYZ.z,
             0.05563008 * XYZ.x - 0.20397696 * XYZ.y + 1.05697151 * XYZ.z
        };
    }

    constexpr glm::dvec3 whitePoint(Illuminant::IDX illuminant, double Y = 1.0)
    {
        return XYZ(Illuminant::xy[illuminant], Y);
    }

    glm::dvec3 whitePoint(std::string illuminant, double Y = 1.0);

    inline constexpr double MIN_WAVELENGTH = 360;
    inline constexpr double MAX_WAVELENGTH = 830;
}
