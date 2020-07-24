#pragma once

#include <glm/vec2.hpp>

#include "cie.hpp"

#include "../common/constexpr-math.hpp"

namespace sRGB
{
    constexpr glm::dmat3 generateRGB2XYZ()
    {
        constexpr glm::dmat3 primaries
        {
            CIE::XYZ({ 0.64, 0.33 }, 1.0), // Red
            CIE::XYZ({ 0.30, 0.60 }, 1.0), // Green
            CIE::XYZ({ 0.15, 0.06 }, 1.0)  // Blue
        };

        constexpr glm::dvec3 white_point = CIE::D65_XYZ / CIE::D65_XYZ[1];

        constexpr glm::dvec3 S = mult(inverse(primaries), white_point);

        return
        {
            S[0] * primaries[0],
            S[1] * primaries[1],
            S[2] * primaries[2]
        };
    }

    // Generated compile-time
    inline constexpr glm::dmat3 RGB2XYZ = generateRGB2XYZ();
    inline constexpr glm::dmat3 XYZ2RGB = inverse(RGB2XYZ);

    // sRGB to XYZ_D65
    constexpr glm::dvec3 XYZ(const glm::dvec3 &RGB)
    {
        return mult(RGB2XYZ, RGB);
    }

    // XYZ_D65 to sRGB
    constexpr glm::dvec3 RGB(const glm::dvec3 &XYZ)
    {
        return mult(XYZ2RGB, XYZ);
    }

    // Spectral distribution to sRGB
    inline glm::dvec3 RGB(const Spectral::Distribution<double> &distribution, Spectral::Type type)
    {
        return RGB(CIE::XYZ(distribution, type));
    }

    inline glm::dvec3 gammaCompress(const glm::dvec3 &in)
    {
        glm::dvec3 out;
        for (uint8_t c = 0; c < 3; c++)
        {
            out[c] = in[c] <= 0.0031308 ? 12.92 * in[c] : 1.055 * std::pow(in[c], 1.0 / 2.4) - 0.055;
        }
        return out;
    }

    inline glm::dvec3 gammaExpand(const glm::dvec3 &in)
    {
        glm::dvec3 out;
        for (uint8_t c = 0; c < 3; c++)
        {
            out[c] = in[c] <= 0.04045 ? in[c] / 12.92 : std::pow((in[c] + 0.055) / 1.055, 2.4);
        }
        return out;
    }
}