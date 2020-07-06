#include "path-tracer.hpp"

#include <glm/gtx/component_wise.hpp>

#include "../../common/util.hpp"
#include "../../random/random.hpp"
#include "../../common/constants.hpp"
#include "../../material/material.hpp"
#include "../../surface/surface.hpp"
#include "../../ray/interaction.hpp"

/*************************************************************************************************************
A material can be any combination of reflective, transparent and diffuse, but instead of branching into several
paths only one is selected probabilistically based on the fresnel and transparency at the intersection point.
Energy is conserved 'automatically' because the path probability is set to be the same as the path weight.
*************************************************************************************************************/
glm::dvec3 PathTracer::sampleRay(Ray ray, size_t ray_depth)
{
    if (ray_depth == Integrator::max_ray_depth)
    {
        Log("Bias introduced: Max ray depth reached in PathTracer::sampleRay()");
        return glm::dvec3(0.0);
    }

    Intersection intersection = scene.intersect(ray);

    if (!intersection)
    {
        return scene.skyColor(ray);
    }

    double absorb = ray_depth > Integrator::min_ray_depth ? 1.0 - intersection.surface->material->reflect_probability : 0.0;

    if (Random::trial(absorb))
    {
        return glm::dvec3(0.0);
    }

    Interaction interaction(intersection, ray, scene.ior);

    glm::dvec3 emittance = (ray_depth == 0 || ray.specular || naive) ? interaction.material->emittance : glm::dvec3(0.0);

    Ray new_ray = interaction.getNewRay();
    glm::dvec3 radiance = sampleRay(new_ray, ray_depth + 1);

    if (interaction.type == Interaction::Type::DIFFUSE)
    {
        radiance *= C::PI;
        if (!naive)
        {
            radiance += Integrator::sampleDirect(interaction);
        }       
    }

    return (emittance + interaction.BRDF(new_ray.direction) * radiance) / (1.0 - absorb);
}