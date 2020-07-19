#include "cie.hpp"

#include <glm/glm.hpp>

#include "illuminant.hpp"
#include "../common/util.hpp"

// Riemann sum with trapezoidal rule
glm::dvec3 CIE::XYZ(const Spectral::Distribution<double> &distribution, Spectral::Type type)
{
    bool is_reflectance = type == Spectral::REFLECTANCE;
    auto i0 = distribution.begin(), i1 = i0, end = distribution.end();

    auto value = [&](double w, auto it) 
    {
        auto v = interpolate(*it, *std::next(it), w) * CMF.get(w);
        return is_reflectance ? v * D65.get(w) : v;
    };

    glm::dvec3 result(0.0);
    for (double w0 = MIN_WAVELENGTH; w0 < MAX_WAVELENGTH; w0 += STEP)
    {
        auto w1 = w0 + STEP;

        i0 = Spectral::advance<double>(i0, end, w0);
        i1 = Spectral::advance<double>(i1, end, w1);

        if (i0 == end || i1 == end) break;

        result += 0.5 * (value(w0, i0) + value(w1, i1)) * STEP;
    }

    return result * (is_reflectance ? NF_REFLECTANCE : NF_RADIANCE);
}

glm::dvec3 CIE::RGB(const Spectral::Distribution<double> &distribution, Spectral::Type type)
{
    return RGB(XYZ(distribution, type));
}

// http://jcgt.org/published/0002/02/01/paper.pdf
glm::dvec3 CIE::fittedCMF(double wavelength)
{
    constexpr auto gaussian = [](double x, double alpha, double mu, double inv_sigma1, double inv_sigma2)
    {
        return alpha * std::exp(-(pow2((x - mu) * (x < mu ? inv_sigma1 : inv_sigma2))) * 0.5);
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