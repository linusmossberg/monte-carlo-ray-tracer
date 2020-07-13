#include "integrator.hpp"

#include <iostream>
#include <thread>

#include <glm/gtx/norm.hpp>

#include "../common/util.hpp"
#include "../common/constants.hpp"
#include "../random/random.hpp"
#include "../material/material.hpp"
#include "../surface/surface.hpp"
#include "../ray/interaction.hpp"

Integrator::Integrator(const nlohmann::json &j) : scene(j)
{
    int threads = getOptional(j, "num_render_threads", -1);
    naive = getOptional(j, "naive", false);

    size_t max_threads = std::thread::hardware_concurrency();
    num_threads = (threads < 1 || threads > max_threads) ? max_threads : threads;
    std::cout << "\nThreads used for rendering: " << num_threads << std::endl;
}

/*****************************************************************************
Only applies cos(theta) from the rendering equation to the diffuse point that 
samples this direct contribution. It must be attenuated by the BRDF later.
******************************************************************************/
glm::dvec3 Integrator::sampleDirect(const Interaction& interaction) const
{
    // Pick one light source and divide with probability of picking light source
    if (!scene.emissives.empty())
    {
        const auto& light = scene.emissives[Random::get<size_t>(0, scene.emissives.size() - 1)];

        glm::dvec3 light_pos = light->operator()(Random::unit(), Random::unit());
        Ray shadow_ray(interaction.position + interaction.normal * C::EPSILON, light_pos);

        double cos_light_theta = glm::dot(-shadow_ray.direction, light->normal(light_pos));

        if (cos_light_theta <= 0.0)
        {
            return glm::dvec3(0.0);
        }

        double cos_theta = glm::dot(shadow_ray.direction, interaction.normal);

        if (cos_theta <= 0)
        {
            return glm::dvec3(0.0);
        }

        Intersection shadow_intersection = scene.intersect(shadow_ray);

        if (shadow_intersection)
        {
            glm::dvec3 position = shadow_ray(shadow_intersection.t);

            if (glm::distance2(position, light_pos) > pow2(C::EPSILON))
            {
                return glm::dvec3(0.0);
            }

            // Factor to transform the PDF of sampling the point on the light (1/area) to 
            // the solid angle PDF at the diffuse point that samples this direct contribution.
            double t = light->area() * cos_light_theta / pow2(shadow_intersection.t);

            return light->material->emittance * t * cos_theta * static_cast<double>(scene.emissives.size());
        }
    }
    return glm::dvec3(0.0);
}