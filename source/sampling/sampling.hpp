#pragma once

#include <random>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "../common/constants.hpp"

namespace Sampling
{
    inline size_t weightedIdx(double u, const std::vector<double>& cdf)
    {
        // Binary search
        size_t left = 0;
        size_t right = cdf.size() - 1;
        while (left < right)
        {
            size_t middle = (left + right) / 2;
            if (cdf[middle] < u)
                left = middle + 1;
            else
                right = middle;
        }
        return left;
    }

    inline glm::dvec2 uniformDisk(double u, double v)
    {
        double azimuth = v * C::TWO_PI;
        return glm::dvec2(std::cos(azimuth), std::sin(azimuth)) * std::sqrt(u);
    }

    inline glm::dvec3 cosWeightedHemi(double u, double v)
    {
        // Generate uniform sample on unit disk at radius r and angle azimuth
        double r = std::sqrt(u);
        double azimuth = v * C::TWO_PI;

        // Project up to hemisphere.
        // z = sin(acos(r)) = sqrt(1-r^2) = sqrt(1-sqrt(u)^2) = sqrt(1-u) 
        return glm::dvec3(r * std::cos(azimuth), r * std::sin(azimuth), std::sqrt(1 - u));
    }
}

namespace Random
{
    // thread_local to create one differently seeded engine per thread
    inline thread_local std::mt19937_64 engine(std::random_device{}());

    inline double unit()
    {
        static thread_local std::uniform_real_distribution<double> unit_distribution(0.0, 1.0);
        return unit_distribution(engine);
    }
}

enum Dim
{
    /* Camera */
    PIXEL = 0, // 2D
    LENS  = 2, // 2D

    /* Bounce (photon bounce only uses some) */
    LIGHT        = 0, // 3D
    BSDF         = 3, // 2D
    INTERACTION  = 5, // 1D
    ABSORB       = 6, // 1D

    /* Photon emission */
    PM_LIGHT = 0, // 4D

    /* Photon bounce */
    PM_REJECT = 2 // 1D
};