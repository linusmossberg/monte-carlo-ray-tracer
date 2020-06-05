#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/component_wise.hpp>

#include <nlohmann/json.hpp>

#include "../common/util.hpp"
#include "../random/random.hpp"
#include "../common/constants.hpp"

class Material
{
public:
    Material()
    {
        roughness = 0.0;
        ior = -1.0;
        transparency = 0.0;
        perfect_mirror = false;
        reflectance = glm::dvec3(0.0);
        specular_reflectance = glm::dvec3(0.0);
        emittance = glm::dvec3(0.0);

        computeProperties();
    }

    glm::dvec3 DiffuseBRDF(const glm::dvec3 &i, const glm::dvec3 &o);
    glm::dvec3 SpecularBRDF();
    glm::dvec3 LambertianBRDF();
    glm::dvec3 OrenNayarBRDF(const glm::dvec3 &i, const glm::dvec3 &o);

    double Fresnel(double n1, double n2, const glm::dvec3& normal, const glm::dvec3& dir) const;

    void computeProperties();

    glm::dvec3 reflectance, specular_reflectance, emittance;
    double roughness, ior, transparency, reflect_probability;

    bool can_diffusely_reflect;

    // Represents ior = infinity -> fresnel factor = 1.0 -> all rays specularly reflected
    bool perfect_mirror;

private:
    // Used for russian roulette path termination
    double calculateReflectProbability();

    // Pre-computed Oren-Nayar variables.
    double A, B;
};

void from_json(const nlohmann::json &j, Material &m);

namespace std
{
    void from_json(const nlohmann::json &j, shared_ptr<Material> &m);
}