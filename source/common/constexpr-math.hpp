#pragma once

#include <glm/matrix.hpp>

template <class T>
constexpr T pow2(T x)
{
    return x * x;
}

constexpr double determinant(const glm::dmat3 &M)
{
    return
    {
        M[0][0] * (M[1][1] * M[2][2] - M[2][1] * M[1][2]) -
        M[1][0] * (M[0][1] * M[2][2] - M[2][1] * M[0][2]) +
        M[2][0] * (M[0][1] * M[1][2] - M[1][1] * M[0][2])
    };
}

constexpr glm::dmat3 inverse(const glm::dmat3 &M)
{
    double inv_det = 1.0 / determinant(M);

    return
    {
        {
            inv_det * (M[1][1] * M[2][2] - M[2][1] * M[1][2]),
            inv_det * (M[2][1] * M[0][2] - M[0][1] * M[2][2]),
            inv_det * (M[0][1] * M[1][2] - M[1][1] * M[0][2])
        },
        {
            inv_det * (M[2][0] * M[1][2] - M[1][0] * M[2][2]),
            inv_det * (M[0][0] * M[2][2] - M[2][0] * M[0][2]),
            inv_det * (M[1][0] * M[0][2] - M[0][0] * M[1][2])
        },
        {
            inv_det * (M[1][0] * M[2][1] - M[2][0] * M[1][1]),
            inv_det * (M[2][0] * M[0][1] - M[0][0] * M[2][1]),
            inv_det * (M[0][0] * M[1][1] - M[1][0] * M[0][1])
        }
    };
}

constexpr glm::dvec3 mult(const glm::dmat3 &M, const glm::dvec3 v)
{
    return
    {
        M[0][0] * v[0] + M[1][0] * v[1] + M[2][0] * v[2],
        M[0][1] * v[0] + M[1][1] * v[1] + M[2][1] * v[2],
        M[0][2] * v[0] + M[1][2] * v[1] + M[2][2] * v[2]
    };
}

constexpr glm::dvec3 mult(const glm::dvec3 &v, double a)
{
    return { v[0] * a, v[1] * a, v[2] * a };
}

constexpr double cfloor(double x)
{
    return static_cast<double>(static_cast<intmax_t>(x));
}

template <class T>
constexpr T cmax(T x, T y)
{
    return x > y ? x : y;
}