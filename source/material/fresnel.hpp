#pragma once

#include <glm/vec3.hpp>

struct ComplexIOR
{
    ComplexIOR(const glm::dvec3 &real, const glm::dvec3 &imaginary) : real(real), imaginary(imaginary) { }
    glm::dvec3 real, imaginary;
};

namespace Fresnel
{
    double SchlickDielectric(double n1, double n2, double cos_theta);
    double dielectric(double n1, double n2, double cos_theta);
    glm::dvec3 conductor(double n1, ComplexIOR* n2, double cos_theta);
}