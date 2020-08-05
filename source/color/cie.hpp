#pragma once

#include <set>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "../common/constexpr-math.hpp"

#include "spectral.hpp"
#include "d65.hpp"
#include "cmf.hpp"

// Using CIE 1931 2°
namespace CIE
{
    // Chromaticity coordinates xy and luminance Y to XYZ
    constexpr glm::dvec3 XYZ(const glm::dvec2 &xy, double Y = 1.0)
    {
        double N = Y / xy[1];
        return { N * xy[0], Y, N * (1.0 - xy[0] - xy[1]) };
    }

    // Spectral radiance L to XYZ using midpoint Riemann sum
    template<unsigned SIZE>
    constexpr glm::dvec3 XYZ(const Spectral::LinearDistribution<double, SIZE> &L)
    {
        glm::dvec3 result(0.0);
        for (double w = CMF.a + 0.5 * CMF.dw; w < CMF.b; w += CMF.dw)
        {
            result += L(w) * CMF(w);
        }
        return CMF.dw * result;
    }

    // Equal energy illuminant
    inline constexpr Spectral::LinearDistribution<double, 2> E({{{ CMF.a, 1.0 },
                                                                 { CMF.b, 1.0 }}});

    // Compile-time integrated tristimulus of spectral radiance from illuminants
    inline constexpr glm::dvec3 D65_XYZ = XYZ<D65.size>(D65);
    inline constexpr glm::dvec3 E_XYZ = XYZ<E.size>(E);

    // Spectral reflectance or radiance distribution to normalized XYZ using midpoint Riemann sum
    inline glm::dvec3 XYZ(const Spectral::Distribution<double> &distribution, Spectral::Type type)
    {
        glm::dvec3 result(0.0);
        auto i = distribution.begin(), end = distribution.end();
        for (double w = CMF.a + 0.5 * CMF.dw; w < CMF.b && Spectral::advance<double>(i, end, w); w += CMF.dw)
        {
            auto v = interpolate(*i, *std::next(i), w) * CMF(w);
            result += type == Spectral::RADIANCE ? v : v * D65(w);
        }
        return (CMF.dw * result) / (type == Spectral::RADIANCE ? E_XYZ.y : D65_XYZ.y);
    }
}
