#pragma once

#include <cmath>
#include "../common/constants.hpp"

// Assumes a radius of 2 and x in [0,2]. The value of any filter with parameter 
// t in [-radius,radius] is therefore given by x = 2*abs(t) / radius
namespace Filter
{
    inline double box(double x)
    {
        return 1.0;
    }

    template<double B = 1.0/3.0, double C = 1.0/3.0>
    inline double MitchellNetravali(double x)
    {
        constexpr double k = 6.0 / (6.0 - 2.0 * B); // Normalization factor
        if (x < 1.0)
        {
            constexpr double a = k * (12.0 - 9.0 * B - 6.0 * C) / 6.0;
            constexpr double b = k * (-18.0 + 12.0 * B + 6.0 * C) / 6.0;
            constexpr double d = k * (6.0 - 2.0 * B) / 6.0;

            return d + (b + a * x) * x * x; // ax^3 + bx^2 + d
        }
        else
        {
            constexpr double a = k * (-B - 6.0 * C) / 6.0;
            constexpr double b = k * (6.0 * B + 30.0 * C) / 6.0;
            constexpr double c = k * (-12.0 * B - 48.0 * C) / 6.0;
            constexpr double d = k * (8.0 * B + 24.0 * C) / 6.0;

            return d + (c + (b + a * x) * x) * x; // ax^3 + bx^2 + cx + d
        }
    }

    inline double CatmullRom(double x)
    {
        return MitchellNetravali<0.0, 0.5>(x);
    }

    inline double BSpline(double x)
    {
        return MitchellNetravali<1.0, 0.0>(x);
    }

    inline double Hermite(double x)
    {
        // Stretch by 2 since Hermite filter loses support for x > 1
        return MitchellNetravali<0.0, 0.0>(x * 0.5);
    }

    inline double Gaussian(double x)
    {
        constexpr double alpha = 2.0;
        static const double exp2 = std::exp(-alpha * 2.0 * 2.0);
        return std::exp(-alpha * x * x) - exp2;
    }

    inline double Lanczos(double x)
    {
        if (x == 0.0) return 1.0;
        return 2.0 * std::sin(C::PI * x) * std::sin(C::PI * x / 2.0) / (C::PI * C::PI * x * x);
    }
}