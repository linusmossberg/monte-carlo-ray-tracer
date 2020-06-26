#include "interaction.hpp"

#include "../material/material.hpp"
#include "../random/random.hpp"
#include "../common/constants.hpp"

Interaction::Type Interaction::type(double n1, double n2, const glm::dvec3& direction) const
{
    double R = material->Fresnel(n1, n2, shading_normal, direction);
    double T = material->transparency;

    double p = Random::range(0, 1);

    if (R > p)
    {
        return Type::REFLECT;
    }
    else if (R + (1 - R) * T > p)
    {
        return Type::REFRACT;
    }
    else // R + (1 - R) * T + (1 - R) * (1 - T) = 1 > p
    {
        return Type::DIFFUSE;
    }
}