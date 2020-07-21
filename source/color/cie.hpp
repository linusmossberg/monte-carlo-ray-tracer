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
    constexpr glm::dvec3 XYZ(const glm::dvec2 &xy, double Y)
    {
        double N = Y / xy[1];
        return { N * xy[0], Y, N * (1.0 - xy[0] - xy[1]) };
    }

    // Spectral radiance L to XYZ using midpoint Riemann sum
    template<unsigned SIZE>
    constexpr glm::dvec3 XYZ(const Spectral::EvenDistribution<double, SIZE> &L)
    {
        glm::dvec3 result(0.0);
        for (double w = CMF.min_w() + CMF.dw() / 2.0; w < CMF.max_w(); w += CMF.dw())
        {
            result += L(w) * CMF(w);
        }
        return CMF.dw() * result;
    }

    // Equal energy illuminant
    inline constexpr Spectral::EvenDistribution<double, 2> E({{{ CMF.min_w(), 1.0 },
                                                               { CMF.max_w(), 1.0 }}});

    // Compile-time integrated tristimulus of spectral radiance from illuminants
    inline constexpr glm::dvec3 D65_XYZ = XYZ<D65.size()>(D65);
    inline constexpr glm::dvec3 E_XYZ = XYZ<E.size()>(E);

    // Spectral reflectance or radiance distribution to XYZ using midpoint Riemann sum
    inline glm::dvec3 XYZ(const Spectral::Distribution<double> &distribution, Spectral::Type type)
    {
        bool is_reflectance = type == Spectral::REFLECTANCE;
        auto it = distribution.begin(), end = distribution.end();

        glm::dvec3 result(0.0);
        for (double w = CMF.min_w() + CMF.dw() / 2.0; w < CMF.max_w(); w += CMF.dw())
        {
            if (!Spectral::advance<double>(it, end, w)) break;
            auto v = interpolate(*it, *std::next(it), w) * CMF(w);
            result += is_reflectance ? v * D65(w) : v;
        }

        return (CMF.dw() * result) / (is_reflectance ? D65_XYZ.y : E_XYZ.y);
    }
}
