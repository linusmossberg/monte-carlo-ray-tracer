#include "path-tracer.hpp"

#include <glm/gtx/component_wise.hpp>

#include "../../common/util.hpp"
#include "../../sampling/sampler.hpp"
#include "../../common/constants.hpp"
#include "../../material/material.hpp"
#include "../../surface/surface.hpp"
#include "../../ray/interaction.hpp"
#include "../../common/constexpr-math.hpp"
#include "../../surface/surface.hpp"

glm::dvec3 PathTracer::sampleRay(Ray ray)
{
    glm::dvec3 radiance(0.0), throughput(1.0);
    RefractionHistory refraction_history(ray);
    glm::dvec3 bsdf_absIdotN;
    LightSample ls;

    while (true)
    {
        Sampler::shuffle();

        Intersection intersection = scene.intersect(ray);

        if (!intersection)
        {
            return radiance + scene.skyColor(ray) * throughput;
        }

        Interaction interaction(intersection, ray, refraction_history.externalIOR(ray));

        radiance += Integrator::sampleEmissive(interaction, ls) * throughput;
        radiance += Integrator::sampleDirect(interaction, ls) * throughput;

        if (!interaction.sampleBSDF(bsdf_absIdotN, ls.bsdf_pdf, ray))
        {
            return radiance;
        }

        throughput *= bsdf_absIdotN / ls.bsdf_pdf;

        if (absorb(ray, throughput))
        {
            return radiance;
        }

        refraction_history.update(ray);
    }
}