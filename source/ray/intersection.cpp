#include "intersection.hpp"

#include "../random/random.hpp"
#include "../common/constants.hpp"

size_t Intersection::selectNewPath(double n1, double n2, const glm::dvec3& direction) const
{
    double R = material->Fresnel(n1, n2, normal, direction);
    double T = material->transparency;

    double p = Random::range(0, 1);

    if (R > p)
    {
        return Path::REFLECT;
    }
    else if (R + (1 - R) * T > p)
    {
        return Path::REFRACT;
    }
    else // R + (1 - R) * T + (1 - R) * (1 - T) = 1 > p
    {
        return Path::DIFFUSE;
    }
}