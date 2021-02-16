#include "integrator.hpp"

#include <iostream>
#include <thread>

#include <glm/gtx/norm.hpp>

#include "../common/util.hpp"
#include "../common/constexpr-math.hpp"
#include "../common/constants.hpp"
#include "../random/random.hpp"
#include "../material/material.hpp"
#include "../surface/surface.hpp"
#include "../ray/interaction.hpp"
#include "../material/fresnel.hpp"

Integrator::Integrator(const nlohmann::json &j) : scene(j)
{
    int threads = getOptional(j, "num_render_threads", -1);

    size_t max_threads = std::thread::hardware_concurrency();
    num_threads = (threads < 1 || threads > max_threads) ? max_threads : threads;
    std::cout << "\nThreads used for rendering: " << num_threads << std::endl;
}

/**************************************************************************
Samples a light source using MIS. The BSDF is sampled using MIS later 
in the next interaction in sampleEmissive, if the ray hits the same light.
**************************************************************************/
glm::dvec3 Integrator::sampleDirect(const Interaction& interaction, LightSample& ls) const
{
    if (scene.emissives.empty() || interaction.material->dirac_delta)
    {
        ls.light = nullptr;
        return glm::dvec3(0.0);
    }

    // Pick one light source and divide with probability of selecting light source
    ls.light = scene.selectLight(ls.select_probability);

    glm::dvec3 light_pos = ls.light->operator()(Random::unit(), Random::unit());
    Ray shadow_ray(interaction.position + interaction.normal * C::EPSILON, light_pos);

    double cos_light_theta = glm::dot(-shadow_ray.direction, ls.light->normal(light_pos));

    if (cos_light_theta <= 0.0)
    {
        return glm::dvec3(0.0);
    }

    double cos_theta = glm::dot(shadow_ray.direction, interaction.normal);
    if (cos_theta <= 0.0)
    {
        if (interaction.material->opaque || cos_theta == 0.0)
        {
            return glm::dvec3(0.0);
        }
        else
        {
            // Try transmission
            shadow_ray = Ray(interaction.position - interaction.normal * C::EPSILON, light_pos);
        }
    }

    Intersection shadow_intersection = scene.intersect(shadow_ray);

    if (!shadow_intersection || shadow_intersection.surface != ls.light)
    {
        return glm::dvec3(0.0);
    }    

    double light_pdf = pow2(shadow_intersection.t) / (ls.light->area() * cos_light_theta);

    double bsdf_pdf;
    glm::dvec3 bsdf_absIdotN;
    if (!interaction.BSDF(bsdf_absIdotN, shadow_ray.direction, bsdf_pdf))
    {
        return glm::dvec3(0.0);
    }

    double mis_weight = powerHeuristic(light_pdf, bsdf_pdf);

    return mis_weight * bsdf_absIdotN * ls.light->material->emittance / (light_pdf * ls.select_probability);
}

/********************************************************************
Adds emittance from interaction surface if applicable, or samples 
the emissive using the BSDF from the previous interaction using MIS.
********************************************************************/
glm::dvec3 Integrator::sampleEmissive(const Interaction& interaction, const LightSample &ls) const
{
    if (interaction.material->emissive && !interaction.inside)
    {
        if (interaction.ray.depth == 0 || interaction.ray.dirac_delta)
        {
            return interaction.material->emittance;
        }
        if(ls.light == interaction.surface)
        {
            double cos_light_theta = glm::dot(interaction.out, interaction.normal);
            double light_pdf = pow2(interaction.t) / (interaction.surface->area() * cos_light_theta);
            double mis_weight = powerHeuristic(ls.bsdf_pdf, light_pdf);
            return mis_weight * interaction.material->emittance / ls.select_probability;
        }
    }
    return glm::dvec3(0.0);
}

bool Integrator::absorb(const Ray &ray, glm::dvec3 &throughput) const
{
    double survive = glm::compMax(throughput) * ray.refraction_scale;

    if (survive == 0.0) return true;

    if (ray.diffuse_depth > min_ray_depth || ray.depth > min_priority_ray_depth)
    {
        survive = std::min(0.95, survive);

        if (!Random::trial(survive))
        {
            return true;
        }
        throughput /= survive;
    }
    return false;
}