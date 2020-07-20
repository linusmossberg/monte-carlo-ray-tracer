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

    double w0 = MIN_WAVELENGTH;
    i0 = Spectral::advance<double>(i0, end, w0);
    if (i0 == end) return glm::dvec3(0.0);
    auto v0 = value(w0, i0);

    glm::dvec3 result(0.0);
    for (double w1 = w0 + STEP; w1 <= MAX_WAVELENGTH; w1 += STEP)
    {
        i1 = Spectral::advance<double>(i1, end, w1);
        if (i1 == end) break;

        auto v1 = value(w1, i1);
        result += v0 + v1;

        w0 = w1; i0 = i1; v0 = v1;
    }

    return STEP * 0.5 * result * (is_reflectance ? NF_REFLECTANCE : NF_RADIANCE);
}

glm::dvec3 CIE::RGB(const Spectral::Distribution<double> &distribution, Spectral::Type type)
{
    return RGB(XYZ(distribution, type));
}