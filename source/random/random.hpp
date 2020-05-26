#pragma once

#include <random>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "../common/constants.hpp"

struct Random
{
    Random() { }

    static unsigned seed();

    static void seed(unsigned seed);

    static std::mt19937_64 getEngine();

    static double range(double v1, double v2);

    static size_t uirange(size_t v1, size_t v2);

    static size_t weightedUIntSample(const std::vector<double>& weights);

    static bool trial(double probability);

    static glm::dvec2 UniformDiskSample();

    static glm::dvec3 CosWeightedHemiSample();
    
    static glm::dvec3 UniformHemiSample();

private:
    // thread_local to create one static random engine per thread.
    thread_local static std::mt19937_64 engine;
    thread_local static unsigned engine_seed;
};
