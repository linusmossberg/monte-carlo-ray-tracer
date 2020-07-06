#pragma once

#include <glm/vec3.hpp>

struct ComplexIOR
{
    glm::dvec3 real, imaginary;
};

namespace Fresnel
{
    double SchlickDielectric(double n1, double n2, double cos_theta);
    double dielectric(double n1, double n2, double cos_theta);
    glm::dvec3 conductor(double n1, ComplexIOR* n2, double cos_theta);
}