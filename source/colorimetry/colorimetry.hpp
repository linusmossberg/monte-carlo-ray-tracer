#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

// Using CIE 1931 2°
namespace CIE
{
    // wavelength to CIE XYZ color matching functions
    glm::dvec3 CMF(double wavelength);

    // Chromaticity coordinates xy and Y to XYZ
    glm::dvec3 XYZ(const glm::dvec2 &xy, double Y);

    // sRGB to XYZ_D65
    glm::dvec3 XYZ(const glm::dvec3 &RGB);

    // XYZ_D65 to sRGB
    glm::dvec3 RGB(const glm::dvec3 &XYZ);

    enum Illuminant {
        A, B, C,
        D50, D55, D65, D75,
        E,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
    };

    glm::dvec3 whitePoint(Illuminant illuminant, double Y = 1.0);
}
