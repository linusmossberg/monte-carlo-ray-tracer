#include "interaction.hpp"

#include "../material/material.hpp"
#include "../random/random.hpp"
#include "../common/constants.hpp"
#include "../common/coordinate-system.hpp"
#include "../surface/surface.hpp"

Interaction::Interaction(const Intersection &isect, const Ray &ray, bool limited)
    : t(isect.t), position(ray(t)), normal(isect.surface->normal(position)), material(isect.surface->material)
{
    if (limited) return;

    double cos_theta = glm::dot(ray.direction, normal);

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

    if (material->rough_specular)
    {
        CoordinateSystem cs(shading_normal);
        specular_normal = cs.localToGlobal(material->specularMicrofacetNormal(cs.globalToLocal(-ray.direction)));
    }
    else
    {
        specular_normal = shading_normal;
    }
}

Interaction::Type Interaction::type(double n1, double n2, const glm::dvec3& direction)
{
    double R = material->Fresnel(n1, n2, specular_normal, direction);
    double T = material->transparency;

    double p = Random::range(0.0, 1.0);

    if (R > p)
    {
        return Type::REFLECT;
    }
    else if (R + (1.0 - R) * T > p)
    {
        return Type::REFRACT;
    }
    else // R + (1 - R) * T + (1 - R) * (1 - T) = 1 > p
    {
        return Type::DIFFUSE;
    }
}