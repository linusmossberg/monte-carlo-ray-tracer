#include "ray.hpp"

Ray::Ray() : start(), direction(), medium_ior(1) { }

Ray::Ray(const glm::dvec3& start) : start(start), direction(), medium_ior(1) { }

Ray::Ray(const glm::dvec3& start, const glm::dvec3& end, double medium_ior)
    : start(start), direction(glm::normalize(end - start)), medium_ior(medium_ior) { }

glm::dvec3 Ray:: operator()(double t) const
{
    return start + direction * t;
}

void Ray::reflectDiffuse(const CoordinateSystem& cs, double n1)
{
    direction = cs.localToGlobal(Random::CosWeightedHemiSample());
    start += cs.normal * offset;
    specular = false;
    medium_ior = n1;
}

void Ray::reflectSpecular(const glm::dvec3 &in, const glm::dvec3 &normal, double n1)
{
    direction = glm::reflect(in, normal);
    start += normal * offset;
    specular = true;
    medium_ior = n1;
}

void Ray::refractSpecular(const glm::dvec3 &in, const glm::dvec3 &normal, double n1, double n2)
{
    specular = true;

    double ior_quotient = n1 / n2;
    double cos_theta = glm::dot(normal, in);
    double k = 1.0 - pow2(ior_quotient) * (1.0 - pow2(cos_theta)); // 1 - (n1/n2 * sin(theta))^2
    if(k >= 0)
    {
        /* SPECULAR REFRACTION */
        direction = ior_quotient * in - (ior_quotient * cos_theta + std::sqrt(k)) * normal;
        start -= normal * offset;
        medium_ior = n2;
    }
    else
    {
        /* CRITICAL ANGLE, SPECULAR REFLECTION */
        direction = in - normal * cos_theta * 2.0;
        start += normal * offset;
        medium_ior = n1;
    }
}