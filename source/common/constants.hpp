#pragma once

namespace C
{
    inline const double PI = 3.14159265358979323846;
    inline const double HALF_PI = 1.57079632679489661923;
    inline const double TWO_PI = 6.283185307179586476925;
    inline const double EPSILON = 1e-7;
}

enum Path
{
    REFLECT,
    REFRACT,
    DIFFUSE
};