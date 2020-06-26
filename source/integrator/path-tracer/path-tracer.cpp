#include "path-tracer.hpp"

#include "../../common/util.hpp"
#include "../../random/random.hpp"
#include "../../common/constants.hpp"
#include "../../material/material.hpp"

/*************************************************************************************************************
A material can be any combination of reflective, transparent and diffuse, but instead of branching into several
paths only one is selected probabilistically based on the fresnel and transparency at the intersection point.
Energy is conserved 'automatically' because the path probability is set to be the same as the path weight.
*************************************************************************************************************/
glm::dvec3 PathTracer::sampleRay(Ray ray, size_t ray_depth)
{
    if (ray_depth == Integrator::max_ray_depth)
    {
        Log("Bias introduced: Max ray depth reached in Scene::sampleRay()");
        return glm::dvec3(0.0);
    }

    Interaction interaction = scene.interact(ray);

    if (!interaction)
    {
        return scene.skyColor(ray);
    }

    double absorb = ray_depth > Integrator::min_ray_depth ? 1.0 - interaction.material->reflect_probability : 0.0;

    if (Random::trial(absorb))
    {
        return glm::dvec3(0.0);
    }

    Ray new_ray(interaction.position);

    glm::dvec3 BRDF;
    glm::dvec3 direct(0);
    glm::dvec3 emittance = (ray_depth == 0 || ray.specular || naive) ? interaction.material->emittance : glm::dvec3(0);

    double n1 = ray.medium_ior;
    double n2 = std::abs(ray.medium_ior - scene.ior) < C::EPSILON ? interaction.material->ior : scene.ior;

    switch (interaction.type(n1, n2, -ray.direction))
    {
        case Interaction::Type::REFLECT:
        {
            BRDF = interaction.material->SpecularBRDF();
            new_ray.reflectSpecular(ray.direction, interaction, n1);
            break;
        }
        case Interaction::Type::REFRACT:
        {
            BRDF = interaction.material->SpecularBRDF();
            new_ray.refractSpecular(ray.direction, interaction, n1, n2);
            break;
        }
        case Interaction::Type::DIFFUSE:
        {
            CoordinateSystem cs(interaction.shading_normal);
            new_ray.reflectDiffuse(cs, interaction, n1);
            BRDF = interaction.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction)) * C::PI;
            if (!naive) direct = Integrator::sampleDirect(interaction);
            break;
        }
    }
    glm::dvec3 indirect = sampleRay(new_ray, ray_depth + 1);

    return (emittance + BRDF * (direct + indirect)) / (1.0 - absorb);
}