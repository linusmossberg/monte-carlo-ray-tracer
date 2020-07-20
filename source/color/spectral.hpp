#pragma once

#include <set>

#include "cie.hpp"
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

    // Returns iterator i where i->wavelength <= w < next(i)->wavelength.
    template<class T>
    typename Distribution<T>::iterator advance(typename Distribution<T>::iterator start,
                                               typename Distribution<T>::iterator end, double w)
    {
        if (start->wavelength >= w && std::next(start) != end)
        {
            return start;
        }
        for (auto it = start; std::next(it) != end; it++)
        {
            if (it->wavelength <= w && std::next(it)->wavelength > w) return it;
        }
        return end;
    };

    template<class T>
    constexpr T interpolate(const Value<T> &s0, const Value<T> &s1, double wavelength)
    {
        double lerp = (wavelength - s0.wavelength) / (s1.wavelength - s0.wavelength);
        return s0.value + lerp * (s1.value - s0.value);
    }

    // constexpr inferring of size and stepsize + automatic static assertion 
    // of valid() would be nice, but I'm not sure if it's possible.
    template<class T, unsigned SIZE, unsigned STEP>
    struct EvenDistribution
    {
        const Value<T> S[SIZE];

        constexpr T get(double wavelength) const
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

        constexpr bool valid() const
        {
            if (SIZE < 2) return false;
            for (int i = 0; i < SIZE - 1; i++)
            {
                double dw = S[i + 1].wavelength - S[i].wavelength;
                double delta = dw < STEP ? STEP - dw : dw - STEP;
                if (dw <= 0.0 || delta > C::EPSILON) return false;
            }
            return true;
        }

        constexpr unsigned size() const { return SIZE; }
        constexpr unsigned step() const { return STEP; }
    };
}