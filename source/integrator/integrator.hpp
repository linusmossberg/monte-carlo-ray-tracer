#pragma once

#include <nlohmann/json.hpp>

#include "../scene/scene.hpp"

class Integrator
{
public:
    Integrator(const nlohmann::json &j);

    virtual ~Integrator() { }

    struct LightSample
    {
        double bsdf_pdf = 0.0, select_probability = 0.0;
        std::shared_ptr<Surface::Base> light;
    };

    virtual glm::dvec3 sampleRay(Ray ray) = 0;
    glm::dvec3 sampleDirect(const Interaction& interaction, LightSample& ls) const;
    glm::dvec3 sampleEmissive(const Interaction& interaction, const LightSample& ls) const;
    bool absorb(const Ray& ray, glm::dvec3& throughput) const;

    size_t num_threads;
    Scene scene;

    const uint8_t min_ray_depth = 3;
    const uint8_t min_priority_ray_depth = 16;
};