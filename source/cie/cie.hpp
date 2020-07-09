#pragma once

#include <set>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

// Using CIE 1931 2°
namespace CIE
{
    // wavelength to CIE XYZ color matching functions
    glm::dvec3 CMF(double wavelength);

    // Chromaticity coordinates xy and luminance Y to XYZ
    glm::dvec3 XYZ(const glm::dvec2 &xy, double Y);

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
    glm::dvec3 XYZ(const glm::dvec3 &RGB);

    // XYZ_D65 to sRGB
    glm::dvec3 RGB(const glm::dvec3 &XYZ);

    enum Illuminant
    {
        A, B, C,
        D50, D55, D65, D75,
        E,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        LED_B1, LED_B2, LED_B3, LED_B4, LED_B5, LED_BH1, LED_RGB1, LED_V1, LED_V2
    };

    glm::dvec3 whitePoint(Illuminant illuminant, double Y = 1.0);

    Illuminant strToIlluminant(std::string &I);

    inline const double MIN_WAVELENGTH = 360;
    inline const double MAX_WAVELENGTH = 830;
}
