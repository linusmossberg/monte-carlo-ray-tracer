#pragma once

#include <set>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "spectral.h"
#include "d65.h"
#include "cmf.h"

// Using CIE 1931 2°
namespace CIE
{
    // Spectral distribution to XYZ
    glm::dvec3 XYZ(const Spectral::Distribution<double> &distribution, Spectral::Type type);

    // Spectral distribution to sRGB
    glm::dvec3 RGB(const Spectral::Distribution<double> &distribution, Spectral::Type type);

    // wavelength to CIE CMF value
    glm::dvec3 fittedCMF(double wavelength);

    // sRGB to XYZ_D65
    constexpr glm::dvec3 XYZ(const glm::dvec3 &RGB)
    {
        return
        {
            0.41239080 * RGB[0] + 0.35758434 * RGB[1] + 0.18048079 * RGB[2],
            0.21263901 * RGB[0] + 0.71516868 * RGB[1] + 0.07219232 * RGB[2],
            0.01933082 * RGB[0] + 0.11919478 * RGB[1] + 0.95053215 * RGB[2]
        };
    }

    // XYZ_D65 to sRGB
    constexpr glm::dvec3 RGB(const glm::dvec3 &XYZ)
    {
        return
        {
             3.24096994 * XYZ[0] - 1.53738318 * XYZ[1] - 0.49861076 * XYZ[2],
            -0.96924364 * XYZ[0] + 1.87596750 * XYZ[1] + 0.04155506 * XYZ[2],
             0.05563008 * XYZ[0] - 0.20397696 * XYZ[1] + 1.05697151 * XYZ[2]
        };
    }

    // Chromaticity coordinates xy and luminance Y to XYZ
    constexpr glm::dvec3 XYZ(const glm::dvec2 &xy, double Y)
    {
        double N = Y / xy[1];
        return { N * xy[0], Y, N * (1.0 - xy[0] - xy[1]) };
    }

    inline constexpr double STEP = 1.0, MIN_WAVELENGTH = 360, MAX_WAVELENGTH = 830;

    // Luminance (Y-component) of spectral radiance L
    template<unsigned L_SIZE, unsigned L_STEP>
    constexpr double luminance(const Spectral::EvenDistribution<double, L_SIZE, L_STEP> &L)
    {
        double result = 0.0;
        for (double w0 = MIN_WAVELENGTH; w0 <= MAX_WAVELENGTH; w0 += STEP)
        {
            double w1 = w0 + STEP;
            result += 0.5 * (CMF.get(w0)[1] * L.get(w0) + CMF.get(w1)[1] * L.get(w1)) * STEP;
        }
        return result;
    }

    // Compile-time integrated normalization factors
    inline constexpr double NF_REFLECTANCE = 1.0 / luminance<D65.size(), D65.step()>(D65);
    inline constexpr double NF_RADIANCE    = 1.0 / luminance<2, 500>({{{ 300, 1.0 },
                                                                       { 800, 1.0 }}});
}
