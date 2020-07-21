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
        double wavelength;
        T value;
        bool operator< (const Value& rhs) const { return wavelength < rhs.wavelength; };
    };

    template<class T>
    using Distribution = std::set<Value<T>>;

    // Advances iterator it such that it->wavelength <= w < next(it)->wavelength.
    template<class T>
    bool advance(typename Distribution<T>::iterator &it,
                 typename Distribution<T>::iterator end, double w)
    {
        if (it->wavelength >= w && std::next(it) != end)
        {
            return true;
        }
        for ( ; std::next(it) != end; it++)
        {
            if (it->wavelength <= w && std::next(it)->wavelength > w) return true;
        }
        return false;
    };

    template<class T>
    constexpr T interpolate(const Value<T> &s0, const Value<T> &s1, double wavelength)
    {
        double lerp = (wavelength - s0.wavelength) / (s1.wavelength - s0.wavelength);
        return s0.value + lerp * (s1.value - s0.value);
    }

    template<class T, unsigned SIZE>
    struct EvenDistribution
    {
        static_assert(SIZE >= 2, "Even spectral distribution must have at least 2 elements.");

        constexpr EvenDistribution(const std::array<Value<T>, SIZE> &S)
            : S(S), STEP(S[1].wavelength - S[0].wavelength)
        {
            if (STEP <= 0.0) throw std::invalid_argument("");

            for (int i = 0; i < SIZE - 1; i++)
            {
                double dw = S[i + 1].wavelength - S[i].wavelength;
                double delta = dw < STEP ? STEP - dw : dw - STEP;
                if (dw <= 0.0 || delta > C::EPSILON)
                {
                    throw std::invalid_argument("");
                }
            }
        }

        constexpr T operator()(double wavelength) const
        {
            if (wavelength < S[0].wavelength || wavelength > S[SIZE - 1].wavelength)
            {
                return T(0);
            }

            unsigned idx = static_cast<unsigned>((wavelength - S[0].wavelength) / STEP);

            if (idx < SIZE - 1)
            {
                return interpolate(S[idx], S[idx + 1], wavelength);
            }

            return S[idx].value;
        }

        constexpr unsigned size() const { return SIZE; }
        constexpr double dw() const { return STEP; }
        constexpr double min_w() const { return S[0].wavelength; }
        constexpr double max_w() const { return S[SIZE-1].wavelength; }

    private:
        const std::array<Value<T>, SIZE> S;
        const double STEP;
    };
}
