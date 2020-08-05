#include "ray.hpp"

#include "../common/constexpr-math.hpp"
#include "../random/random.hpp"
#include "../common/constants.hpp"
#include "interaction.hpp"

Ray::Ray(const glm::dvec3& start, const glm::dvec3& end, double medium_ior)
    : start(start), direction(glm::normalize(end - start)), medium_ior(medium_ior) { }

Ray::Ray(const Interaction &ia)
    : start(ia.position), depth(ia.ray.depth + 1), diffuse_depth(ia.ray.diffuse_depth)
{
    switch (ia.type)
    {
        case Interaction::REFLECT:
        {
            specular = true;
            direction = glm::reflect(ia.ray.direction, ia.cs.normal);
            medium_ior = ia.n1;
            start += ia.normal * C::EPSILON;
            break;
        }
        case Interaction::REFRACT:
        {
            specular = true;

            double ior_quotient = ia.n1 / ia.n2;
            double cos_theta = glm::dot(ia.cs.normal, ia.ray.direction);
            double k = 1.0 - pow2(ior_quotient) * (1.0 - pow2(cos_theta)); // 1 - (n1/n2 * sin(theta))^2
            if (k >= 0)
            {
                /* SPECULAR REFRACTION */
                direction = ior_quotient * ia.ray.direction - (ior_quotient * cos_theta + std::sqrt(k)) * ia.cs.normal;
                medium_ior = ia.n2;
                start -= ia.normal * C::EPSILON;
            }
            else
            {
                /* CRITICAL ANGLE, SPECULAR REFLECTION */
                direction = ia.ray.direction - ia.cs.normal * cos_theta * 2.0;
                medium_ior = ia.n1;
                start += ia.normal * C::EPSILON;
            }
            break;
        }
        case Interaction::DIFFUSE:
        {
            specular = false;
            diffuse_depth++;
            direction = ia.cs.from(Random::cosWeightedHemiSample());
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