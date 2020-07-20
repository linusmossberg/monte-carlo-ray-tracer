#include "cie.hpp"

#include <glm/glm.hpp>

#include "illuminant.hpp"
#include "../common/util.hpp"

// Trapezoidal rule Riemann sum
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