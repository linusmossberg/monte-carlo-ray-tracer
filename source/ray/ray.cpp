#include "ray.hpp"

#include "../common/constexpr-math.hpp"
#include "../random/random.hpp"
#include "../common/constants.hpp"
#include "interaction.hpp"

Ray::Ray() : start(), direction(), medium_ior(1.0) { }

Ray::Ray(const glm::dvec3& start, uint8_t depth, uint8_t diffuse_depth) 
    : start(start), depth(depth), diffuse_depth(diffuse_depth), direction(), medium_ior(1.0) { }

Ray::Ray(const glm::dvec3& start, const glm::dvec3& end, double medium_ior)
    : start(start), direction(glm::normalize(end - start)), medium_ior(medium_ior) { }

glm::dvec3 Ray:: operator()(double t) const
{
    return start + direction * t;
}

void Ray::reflectDiffuse(const Interaction &ia)
{
    specular = false;
    depth++;
    diffuse_depth++;
    direction = ia.cs.from(Random::cosWeightedHemiSample());
    medium_ior = ia.n1;
    start += ia.normal * C::EPSILON;
}

void Ray::reflectSpecular(const glm::dvec3 &in, const Interaction &ia)
{
    specular = true;
    depth++;
    direction = glm::reflect(in, ia.cs.normal);
    medium_ior = ia.n1;
    start += ia.normal * C::EPSILON;
}

void Ray::refractSpecular(const glm::dvec3 &in, const Interaction &ia)
{
    specular = true;
    depth++;

    double ior_quotient = ia.n1 / ia.n2;
    double cos_theta = glm::dot(ia.cs.normal, in);
    double k = 1.0 - pow2(ior_quotient) * (1.0 - pow2(cos_theta)); // 1 - (n1/n2 * sin(theta))^2
    if (k >= 0)
    {
        /* SPECULAR REFRACTION */
        direction = ior_quotient * in - (ior_quotient * cos_theta + std::sqrt(k)) * ia.cs.normal;
        medium_ior = ia.n2;
        start -= ia.normal * C::EPSILON;
    }
    else
    {
        /* CRITICAL ANGLE, SPECULAR REFLECTION */
        direction = in - ia.cs.normal * cos_theta * 2.0;
        medium_ior = ia.n1;
        start += ia.normal * C::EPSILON;
    }
}