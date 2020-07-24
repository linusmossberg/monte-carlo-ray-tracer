#pragma once

#include <set>
#include <array>
#include <stdexcept>

#include "../common/constants.hpp"

namespace Spectral
{
    enum Type { REFLECTANCE, RADIANCE };

    template<class T>
    struct Value
    {
        double w; // wavelength
        T value;
        bool operator< (const Value& rhs) const { return w < rhs.w; };
    };

    template<class T>
    using Distribution = std::set<Value<T>>;

    // Advances iterator it such that it->w <= w < next(it)->w.
    template<class T>
    bool advance(typename Distribution<T>::iterator &it,
                 typename Distribution<T>::iterator end, double w)
    {
        if (it->w >= w && std::next(it) != end)
        {
            return true;
        }
        for ( ; std::next(it) != end; it++)
        {
            if (it->w <= w && std::next(it)->w > w) return true;
        }
        return false;
    };

    template<class T>
    constexpr T interpolate(const Value<T> &s0, const Value<T> &s1, double w)
    {
        double lerp = (w - s0.w) / (s1.w - s0.w);
        if (lerp < 0.0 || lerp > 1.0) return T(0);
        return s0.value + lerp * (s1.value - s0.value);
    }

    template<class T, size_t SIZE>
    struct EvenDistribution
    {
        static_assert(SIZE >= 2, "Even spectral distribution must have at least 2 elements.");

        constexpr EvenDistribution(const std::array<Value<T>, SIZE> &S)
            : S(S), dw(S[1].w - S[0].w), a(S[0].w), b(S[SIZE - 1].w)
        {
            if (dw <= 0.0) throw std::invalid_argument("");

            for (int i = 0; i < SIZE - 1; i++)
            {
                double step = S[i + 1].w - S[i].w;
                double delta = step < dw ? dw - step : step - dw;
                if (step <= 0.0 || delta > C::EPSILON)
                {
                    throw std::invalid_argument("");
                }
            }
        }

        constexpr T operator()(double w) const
        {
            if (w < a || w > b) return T(0);

            size_t i = static_cast<size_t>((w - a) / dw);

            return i < SIZE - 1 ? interpolate(S[i], S[i + 1], w) : S[i].value;
        }

        const double a;  // Start wavelength
        const double b;  // End wavelength
        const double dw; // Wavelength step size

        const size_t size = SIZE;

        // Common start Riemann sum midpoint wavelength aligned with this distribution
        constexpr double aMid(double second_a) const
        {
            return a + (cfloor((cmax(second_a, a) - a) / dw) + 0.5) * dw;
        }

    private:
        const std::array<Value<T>, SIZE> S;
    };
}
