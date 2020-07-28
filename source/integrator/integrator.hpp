#pragma once

#include <nlohmann/json.hpp>

#include "../scene/scene.hpp"

class Integrator
{
public:
    Integrator(const nlohmann::json &j);

    virtual ~Integrator() { }

    virtual glm::dvec3 sampleRay(Ray ray) = 0;
    virtual glm::dvec3 sampleDirect(const Interaction& interaction) const;
    bool absorb(const Ray &ray, const Intersection &isect, double &survive) const;

    bool naive;
    size_t num_threads;
    Scene scene;

    const size_t min_ray_depth = 3;
    const size_t min_priority_ray_depth = 16;
    const size_t max_ray_depth = 96; // prevent call stack overflow
};