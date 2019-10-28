#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/component_wise.hpp>

#include "../common/util.hpp"
#include "../random/random.hpp"
#include "../common/constants.hpp"

class Material
{
public:
    Material(
        const glm::dvec3& reflectance, 
        const glm::dvec3& specular_reflectance, 
        const glm::dvec3& emission, 
        double roughness, double ior, 
        double transparency, bool perfect_mirror, 
        double scene_ior) 
        : 
        reflectance(reflectance),
        specular_reflectance(specular_reflectance),
        emittance(emission), roughness(roughness), 
        ior(ior), transparency(transparency),
        perfect_mirror(perfect_mirror)
    {
        can_diffusely_reflect = !perfect_mirror && std::abs(transparency - 1.0) > C::EPSILON;
        reflect_probability = calculateReflectProbability(scene_ior);
    }

    glm::dvec3 DiffuseBRDF(const glm::dvec3 &i, const glm::dvec3 &o);
    glm::dvec3 SpecularBRDF();
    glm::dvec3 LambertianBRDF();
    glm::dvec3 OrenNayarBRDF(const glm::dvec3 &i, const glm::dvec3 &o);

    double Fresnel(double n1, double n2, const glm::dvec3& normal, const glm::dvec3& dir) const;

    size_t selectPath(double n1, double n2, const glm::dvec3& normal, const glm::dvec3& dir) const;

    glm::dvec3 reflectance, specular_reflectance, emittance;
    double roughness, ior, transparency, reflect_probability;

    bool can_diffusely_reflect;

    // Represents ior = infinity -> fresnel factor = 1.0 -> all rays specularly reflected
    bool perfect_mirror;

private:
    // Used for russian roulette path termination
    double calculateReflectProbability(double scene_ior);
};