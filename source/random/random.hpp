#pragma once

#include <random>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace Random
{
    // thread_local to create one differently seeded engine per thread
    inline thread_local std::mt19937_64 engine(std::random_device{}());

    template <typename T>
    T get(const T min, const T max) 
    {
        if constexpr (std::is_integral<T>::value)
            return std::uniform_int_distribution(min, max)(engine); // [min,max]
        else
            return std::uniform_real_distribution(min, std::nextafter(max, min))(engine); // [min,max)
    }

    // Random::get with commonly used distributions. 
    // These may be possible to non-type template in C++20
    double unit(), angle();

    size_t weightedIdxSample(const std::vector<double>& cumulative_weights);

    bool trial(double probability);

    glm::dvec2 uniformDiskSample();

    glm::dvec3 cosWeightedHemiSample();
   
    glm::dvec3 uniformHemiSample();
}