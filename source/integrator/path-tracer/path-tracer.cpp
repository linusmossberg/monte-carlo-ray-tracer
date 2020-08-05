#include "path-tracer.hpp"

#include <glm/gtx/component_wise.hpp>

#include "../../common/util.hpp"
#include "../../random/random.hpp"
#include "../../common/constants.hpp"
#include "../../material/material.hpp"
#include "../../surface/surface.hpp"
#include "../../ray/interaction.hpp"

glm::dvec3 PathTracer::sampleRay(Ray ray)
{
    if (ray.depth == Integrator::max_ray_depth)
    {
        Log("Bias introduced: Max ray depth reached in PathTracer::sampleRay()");
        return glm::dvec3(0.0);
    }

    Intersection intersection = scene.intersect(ray);

    if (!intersection)
    {
        return scene.skyColor(ray);
    }

    double survive;
    if (absorb(ray, intersection, survive))
    {
        return glm::dvec3(0.0);
    }

    Interaction interaction(intersection, ray);

    glm::dvec3 emittance = (ray.depth == 0 || ray.specular || naive) ? interaction.material->emittance : glm::dvec3(0.0);

    Ray new_ray(interaction);
    glm::dvec3 radiance = sampleRay(new_ray);

    if (interaction.type == Interaction::Type::DIFFUSE)
    {
        radiance *= C::PI;
        if (!naive)
        {
            radiance += Integrator::sampleDirect(interaction);
        }       
    }

    return (emittance + interaction.BRDF(new_ray.direction) * radiance) / survive;
}