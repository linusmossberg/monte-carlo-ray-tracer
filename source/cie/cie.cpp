#include "cie.hpp"

#include <glm/glm.hpp>

#include "../common/util.hpp"

// http://jcgt.org/published/0002/02/01/paper.pdf
glm::dvec3 CIE::CMF(double wavelength)
{
    auto gaussian = [](double x, double alpha, double mu, double inv_sigma1, double inv_sigma2)
    {
        return alpha * exp(-(pow2((x - mu) * (x < mu ? inv_sigma1 : inv_sigma2))) * 0.5);
    };

    return
    {
        gaussian(wavelength,  0.362, 442.0, 0.0624, 0.0374) +
        gaussian(wavelength,  1.056, 599.8, 0.0264, 0.0323) +
        gaussian(wavelength, -0.065, 501.1, 0.0490, 0.0382),

        gaussian(wavelength,  0.821, 568.8, 0.0213, 0.0247) +
        gaussian(wavelength,  0.286, 530.9, 0.0613, 0.0322),

        gaussian(wavelength,  1.217, 437.0, 0.0845, 0.0278) +
        gaussian(wavelength,  0.681, 459.0, 0.0385, 0.0725)
    };
}

glm::dvec3 CIE::XYZ(const glm::dvec2 &xy, double Y)
{
    double N = Y / xy.y;
    return { N * xy.x, Y, N * (1.0 - xy.x - xy.y) };
}

glm::dvec3 CIE::XYZ(const glm::dvec3 &RGB)
{
    return 
    {
        0.41239080 * RGB.r + 0.35758434 * RGB.g + 0.18048079 * RGB.b,
        0.21263901 * RGB.r + 0.71516868 * RGB.g + 0.07219232 * RGB.b,
        0.01933082 * RGB.r + 0.11919478 * RGB.g + 0.95053215 * RGB.b
    };
}

glm::dvec3 CIE::XYZ(const std::set<SpectralValue> &distribution)
{
    auto it = distribution.begin();
    SpectralValue s0 = *it;
    glm::dvec3 xyz0 = CMF(s0.wavelength);
    it++;

    glm::dvec3 result;
    double Y_integral = 0.0;
    for ( ; it != distribution.end(); it++)
    {
        if (s0.wavelength > CIE::MAX_WAVELENGTH) break;

        SpectralValue s1 = *it;
        glm::dvec3 xyz1 = CMF(s1.wavelength);

        if (s1.wavelength > CIE::MIN_WAVELENGTH)
        {
            double dw = s1.wavelength - s0.wavelength;
            result += (s0.value * xyz0 + s1.value * xyz1) * 0.5 * dw;
            Y_integral += (xyz0.y + xyz1.y) * 0.5 * dw;
        }

        s0 = s1;
        xyz0 = xyz1;
    }
    return result / Y_integral;
}

glm::dvec3 CIE::RGB(const std::set<SpectralValue> &distribution)
{
    return RGB(XYZ(distribution) * whitePoint(Illuminant::D65));
}

glm::dvec3 CIE::RGB(const glm::dvec3 &XYZ)
{
    return 
    {
         3.24096994 * XYZ.x - 1.53738318 * XYZ.y - 0.49861076 * XYZ.z,
        -0.96924364 * XYZ.x + 1.87596750 * XYZ.y + 0.04155506 * XYZ.z,
         0.05563008 * XYZ.x - 0.20397696 * XYZ.y + 1.05697151 * XYZ.z
    };
}

glm::dvec3 CIE::whitePoint(Illuminant illuminant, double Y)
{
    switch(illuminant)
    {
        case Illuminant::A:        return XYZ({0.44757, 0.40745}, Y);
        case Illuminant::B:        return XYZ({0.34842, 0.35161}, Y);
        case Illuminant::C:        return XYZ({0.31006, 0.31616}, Y);
        case Illuminant::D50:      return XYZ({0.34567, 0.35850}, Y);
        case Illuminant::D55:      return XYZ({0.33242, 0.34743}, Y);
        case Illuminant::D65:      return XYZ({0.31271, 0.32902}, Y);
        case Illuminant::D75:      return XYZ({0.29902, 0.31485}, Y);
        case Illuminant::E:        return XYZ({1.0/3.0, 1.0/3.0}, Y);
        case Illuminant::F1:       return XYZ({0.31310, 0.33727}, Y);
        case Illuminant::F2:       return XYZ({0.37208, 0.37529}, Y);
        case Illuminant::F3:       return XYZ({0.40910, 0.39430}, Y);
        case Illuminant::F4:       return XYZ({0.44018, 0.40329}, Y);
        case Illuminant::F5:       return XYZ({0.31379, 0.34531}, Y);
        case Illuminant::F6:       return XYZ({0.37790, 0.38835}, Y);
        case Illuminant::F7:       return XYZ({0.31292, 0.32933}, Y);
        case Illuminant::F8:       return XYZ({0.34588, 0.35875}, Y);
        case Illuminant::F9:       return XYZ({0.37417, 0.37281}, Y);
        case Illuminant::F10:      return XYZ({0.34609, 0.35986}, Y);
        case Illuminant::F11:      return XYZ({0.38052, 0.37713}, Y);
        case Illuminant::F12:      return XYZ({0.43695, 0.40441}, Y);
        case Illuminant::LED_B1:   return XYZ({0.45600, 0.40780}, Y);
        case Illuminant::LED_B2:   return XYZ({0.43570, 0.40120}, Y);
        case Illuminant::LED_B3:   return XYZ({0.37560, 0.37230}, Y);
        case Illuminant::LED_B4:   return XYZ({0.34220, 0.35020}, Y);
        case Illuminant::LED_B5:   return XYZ({0.31180, 0.32360}, Y);
        case Illuminant::LED_BH1:  return XYZ({0.44740, 0.40660}, Y);
        case Illuminant::LED_RGB1: return XYZ({0.45570, 0.42110}, Y);
        case Illuminant::LED_V1:   return XYZ({0.45600, 0.45480}, Y);
        case Illuminant::LED_V2:   return XYZ({0.37810, 0.37750}, Y);
    }
    return glm::dvec3(Y);
}

CIE::Illuminant CIE::strToIlluminant(std::string &I)
{
    std::transform(I.begin(), I.end(), I.begin(), toupper);

    if (I == "A")        return Illuminant::A;
    if (I == "B")        return Illuminant::B;
    if (I == "C")        return Illuminant::C;
    if (I == "D50")      return Illuminant::D50;
    if (I == "D55")      return Illuminant::D55;
    if (I == "D65")      return Illuminant::D65;
    if (I == "D75")      return Illuminant::D75;
    if (I == "E")        return Illuminant::E;
    if (I == "F1")       return Illuminant::F1;
    if (I == "F2")       return Illuminant::F2;
    if (I == "F3")       return Illuminant::F3;
    if (I == "F4")       return Illuminant::F4;
    if (I == "F5")       return Illuminant::F5;
    if (I == "F6")       return Illuminant::F6;
    if (I == "F7")       return Illuminant::F7;
    if (I == "F8")       return Illuminant::F8;
    if (I == "F9")       return Illuminant::F9;
    if (I == "F10")      return Illuminant::F10;
    if (I == "F11")      return Illuminant::F11;
    if (I == "F12")      return Illuminant::F12;
    if (I == "LED-B1")   return Illuminant::LED_B1;
    if (I == "LED-B2")   return Illuminant::LED_B2;
    if (I == "LED-B3")   return Illuminant::LED_B3;
    if (I == "LED-B4")   return Illuminant::LED_B4;
    if (I == "LED-B5")   return Illuminant::LED_B5;
    if (I == "LED-BH1")  return Illuminant::LED_BH1;
    if (I == "LED-RGB1") return Illuminant::LED_RGB1;
    if (I == "LED-V1")   return Illuminant::LED_V1;
    if (I == "LED-V2")   return Illuminant::LED_V2;

    return Illuminant::D65;
}