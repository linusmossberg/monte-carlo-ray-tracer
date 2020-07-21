#pragma once

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

struct ComplexIOR
{
    ComplexIOR() : real(0.0), imaginary(0.0) { }
    ComplexIOR(const glm::dvec3 &real, const glm::dvec3 &imaginary) : real(real), imaginary(imaginary) { }
    glm::dvec3 real, imaginary;
};

namespace Fresnel
{
    double dielectric(double n1, double n2, double cos_theta);
    glm::dvec3 conductor(double n1, ComplexIOR* n2, double cos_theta);
}

void from_json(const nlohmann::json &j, ComplexIOR &m);