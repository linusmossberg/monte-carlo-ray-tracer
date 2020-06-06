#pragma once

#include <nlohmann/json.hpp>

#include "../scene/scene.hpp"

class Integrator
{
public:
    Integrator(const nlohmann::json &j) : scene(j)
    {
        int threads = getOptional(j, "num_render_threads", -1);
        naive = getOptional(j, "naive", false);

        size_t max_threads = std::thread::hardware_concurrency();
        num_threads = (threads < 1 || threads > max_threads) ? max_threads : threads;
        std::cout << "Threads used for rendering: " << num_threads << std::endl << std::endl;
    }

    virtual glm::dvec3 sampleRay(Ray ray, size_t ray_depth = 0) = 0;

    virtual glm::dvec3 sampleDirect(const Intersection& intersection) const;

    bool naive;

    size_t num_threads;

    Scene scene;

    const size_t min_ray_depth = 3;
    const size_t max_ray_depth = 96; // prevent call stack overflow
};