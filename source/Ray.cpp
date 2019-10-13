#include "Ray.hpp"

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
    direction = glm::refract(in, normal, n1 / n2);
    if (std::isnan(direction.x))
    {
        /* BREWSTER ANGLE, SPECULAR REFLECTION */
        reflectSpecular(in, normal, n1);
    }
    else
    {
        /* SPECULAR REFRACTION */
        start -= normal * offset;
        specular = true;
        medium_ior = n2;
    }
}