#include "ray.hpp"

#include "../common/constexpr-math.hpp"
#include "../random/random.hpp"
#include "../common/constants.hpp"
#include "../material/material.hpp"
#include "interaction.hpp"

Ray::Ray(const glm::dvec3& start, const glm::dvec3& end, double medium_ior)
    : start(start), direction(glm::normalize(end - start)), medium_ior(medium_ior) { }

Ray::Ray(const Interaction &ia) : 
    depth(ia.ray.depth + 1), diffuse_depth(ia.ray.diffuse_depth),
    refraction_scale(ia.ray.refraction_scale), start(ia.position),
    refraction_level(ia.ray.refraction_level), dirac_delta(ia.dirac_delta)
{
    switch (ia.type)
    {
        case Interaction::REFLECT:
        {
            direction = glm::reflect(ia.ray.direction, ia.specular_normal);
            medium_ior = ia.n1;
            start += ia.normal * C::EPSILON;
            break;
        }
        case Interaction::REFRACT:
        {
            double inv_eta = ia.n1 / ia.n2;
            double cos_theta = glm::dot(ia.specular_normal, ia.ray.direction);
            double k = 1.0 - pow2(inv_eta) * (1.0 - pow2(cos_theta)); // 1 - (n1/n2 * sin(theta))^2
            if (k >= 0.0)
            {
                /* SPECULAR REFRACTION */
                direction = inv_eta * ia.ray.direction - (inv_eta * cos_theta + std::sqrt(k)) * ia.specular_normal;
                medium_ior = ia.n2;
                start -= ia.normal * C::EPSILON;
                ia.inside ? refraction_level-- : refraction_level++;
                refraction_scale *= pow2(1.0 / inv_eta);
                refraction = true;
            }
            else
            {
                /* CRITICAL ANGLE, SPECULAR REFLECTION */
                direction = ia.ray.direction - ia.specular_normal * cos_theta * 2.0;
                medium_ior = ia.n1;
                start += ia.normal * C::EPSILON;
            }
            break;
        }
        case Interaction::DIFFUSE:
        {
            diffuse_depth++;
            direction = ia.shading_cs.from(Random::cosWeightedHemiSample());
            medium_ior = ia.n1;
            start += ia.normal * C::EPSILON;
            break;
        }
    }
}

glm::dvec3 Ray:: operator()(double t) const
{
    return start + direction * t;
}

RefractionHistory::RefractionHistory(const Ray& ray) :
    iors(1, ray.medium_ior)
{
    iors.reserve(8);
}

void RefractionHistory::update(const Ray& ray)
{
    if (ray.refraction_level > 0)
    {
        if (ray.refraction_level == iors.size())
        {
            iors.push_back(ray.medium_ior);
        }
        else if (ray.refraction_level < iors.size() - 1)
        {
            iors.pop_back();
        }
    }
}

double RefractionHistory::externalIOR(const Ray& ray) const
{
    return iors[std::clamp(ray.refraction_level - 1, 0, (int)iors.size() - 1)];
}