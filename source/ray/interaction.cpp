#include "interaction.hpp"

#include "../material/fresnel.hpp"
#include "../material/material.hpp"
#include "../random/random.hpp"
#include "../common/constants.hpp"
#include "../common/coordinate-system.hpp"
#include "../surface/surface.hpp"

Interaction::Interaction(const Intersection &isect, const Ray &ray, double environment_ior)
    : t(isect.t), position(ray(t)), normal(isect.surface->normal(position)), 
      material(isect.surface->material), out(-ray.direction), n1(ray.medium_ior)
{
    n2 = std::abs(n1 - environment_ior) < C::EPSILON ? material->ior : environment_ior;
    double cos_theta = glm::dot(ray.direction, normal);

    glm::dvec3 shading_normal;
    if (isect.interpolate)
    {
        shading_normal = isect.surface->interpolatedNormal(isect.uv);
        if (cos_theta < 0.0 != glm::dot(ray.direction, shading_normal) < 0.0)
        {
            shading_normal = normal;
        }
    }
    else
    {
        shading_normal = normal;
    }

    if (cos_theta > 0.0)
    {
        normal = -normal;
        shading_normal = -shading_normal;
    }

    cs = CoordinateSystem(shading_normal);

    if (material->rough_specular)
    {
        glm::dvec3 specular_normal = cs.from(material->specularMicrofacetNormal(cs.to(out)));
        selectType(specular_normal);
        if (type != DIFFUSE) cs = CoordinateSystem(specular_normal);
    }
    else
    {
        selectType(shading_normal);
    }
}

void Interaction::selectType(const glm::dvec3 &specular_normal)
{
    if (material->perfect_mirror || material->complex_ior)
    {
        type = REFLECT; 
        return;
    }

    double R = Fresnel::dielectric(n1, n2, glm::dot(specular_normal, out));
    double T = material->transparency;

    double p = Random::range(0.0, 1.0);

    if (R > p)
    {
        type = REFLECT;
    }
    else if (R + (1.0 - R) * T > p)
    {
        type = REFRACT;
    }
    else // R + (1 - R) * T + (1 - R) * (1 - T) = 1 > p
    {
        type = DIFFUSE;
    }
}

glm::dvec3 Interaction::BRDF(const glm::dvec3 &in) const
{
    glm::dvec3 local_in = cs.to(in);
    if (local_in.z == 0.0) return glm::dvec3(0.0); // Grazing angle edge case

    if (type != DIFFUSE)
    {
        glm::dvec3 local_out = cs.to(out);
        glm::dvec3 brdf = material->SpecularBRDF(local_in, local_out);
        if (material->complex_ior)
        {
            brdf *= Fresnel::conductor(n1, material->complex_ior.get(), local_out.z);
        }
        return brdf;
    }
    else
    {
        return material->DiffuseBRDF(local_in, cs.to(out));
    }
}

Ray Interaction::getNewRay() const
{
    Ray new_ray(position);
    switch (type)
    {
        case REFLECT:
        {
            new_ray.reflectSpecular(-out, *this); 
            break;
        }
        case REFRACT:
        {
            new_ray.refractSpecular(-out, *this); 
            break;
        }
        case DIFFUSE:
        {
            new_ray.reflectDiffuse(*this);
            break;
        }
    }
    return new_ray;
}