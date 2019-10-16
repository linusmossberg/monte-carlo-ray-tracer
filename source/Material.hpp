#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/component_wise.hpp>

#include "Util.hpp"

class Material
{
public:
    Material() : reflectance(0.8), specular_reflectance(0.0), emittance(0.0) { }

    Material(const glm::dvec3& reflectance, const glm::dvec3& specular_reflectance, const glm::dvec3& emission, double roughness, double ior, double transparency, bool perfect_mirror) 
        : reflectance(reflectance),
        specular_reflectance(specular_reflectance),
        emittance(emission),
        roughness(roughness), 
        ior(ior),
        transparency(transparency),
        perfect_mirror(perfect_mirror)
    {
        reflect_probability = calculateReflectProbability();
    }

    glm::dvec3 DiffuseBRDF(const glm::dvec3 &i, const glm::dvec3 &o);
    glm::dvec3 SpecularBRDF();
    glm::dvec3 LambertianBRDF();
    glm::dvec3 OrenNayarBRDF(const glm::dvec3 &i, const glm::dvec3 &o);

    // Schlick's approximation of fresnel factor
    static double Fresnel(double n1, double n2, const glm::dvec3& normal, const glm::dvec3& dir)
    {
        if (abs(n1 - n2) < 1e-7)
            return 0;

        double R0 = pow2((n1 - n2) / (n1 + n2));
        return R0 + (1.0 - R0) * pow(1.0 - glm::dot(normal, dir), 5);
    }

    glm::dvec3 reflectance, specular_reflectance, emittance;
    double roughness, ior, transparency, reflect_probability;

    // Represents ior = infinity -> fresnel factor = 1.0 -> all rays specularly reflected
    bool perfect_mirror;

private:
    // Used for russian roulette path termination
    double calculateReflectProbability();
};