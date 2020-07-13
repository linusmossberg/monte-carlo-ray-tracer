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
    return RGB(XYZ(distribution) * whitePoint(Illuminant::IDX::D65));
}


glm::dvec3 CIE::whitePoint(std::string illuminant, double Y)
{
    std::transform(illuminant.begin(), illuminant.end(), illuminant.begin(), toupper);
    return XYZ(Illuminant::xy[Illuminant::fromString(illuminant)], Y);
}