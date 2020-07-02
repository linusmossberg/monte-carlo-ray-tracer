#include "integrator.hpp"

#include <iostream>
#include <thread>

#include "../common/util.hpp"
#include "../common/constants.hpp"
#include "../random/random.hpp"
#include "../material/material.hpp"
#include "../surface/surface.hpp"

Integrator::Integrator(const nlohmann::json &j) : scene(j)
{
    int threads = getOptional(j, "num_render_threads", -1);
    naive = getOptional(j, "naive", false);

    size_t max_threads = std::thread::hardware_concurrency();
    num_threads = (threads < 1 || threads > max_threads) ? max_threads : threads;
    std::cout << "\nThreads used for rendering: " << num_threads << std::endl;
}

/**********************************************************************************************
Only applies cos(theta) from the rendering equation to the diffuse point that samples this
direct contribution. This way direct and indirect (which uses cosine weighted sampling with
PDF = cos(theta) / PI) can be treated the same way later, i.e. (direct + indirect) * (BRDF*PI).
We also need to divide by PI to make (BRDF*PI)  applicable to the direct contribution as well.
**********************************************************************************************/
glm::dvec3 Integrator::sampleDirect(const Interaction& interaction) const
{
    // Pick one light source and divide with probability of picking light source
    if (!scene.emissives.empty())
    {
        const auto& light = scene.emissives[Random::uirange(0, scene.emissives.size() - 1)];

        glm::dvec3 light_pos = light->operator()(Random::range(0, 1), Random::range(0, 1));
        Ray shadow_ray(interaction.position + interaction.normal * C::EPSILON, light_pos);

        double cos_theta = glm::dot(shadow_ray.direction, interaction.normal);

        if (cos_theta <= 0)
        {
            return glm::dvec3(0.0);
        }    

        Interaction shadow_interaction = scene.interact(shadow_ray, true);

        if (shadow_interaction)
        {
            double cos_light_theta = glm::dot(shadow_interaction.normal, -shadow_ray.direction);

            if (cos_light_theta <= 0 || glm::distance(shadow_interaction.position, light_pos) > C::EPSILON)
            {
                return glm::dvec3(0.0);
            }

            // Factor to transform the PDF of sampling the point on the light (1/area) to 
            // the solid angle PDF at the diffuse point that samples this direct contribution.
            double t = light->area() * cos_light_theta / pow2(shadow_interaction.t);

            return (light->material->emittance * t * cos_theta * static_cast<double>(scene.emissives.size())) / C::PI;
        }
    }
    return glm::dvec3(0.0);
}