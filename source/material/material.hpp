#pragma once

#include <functional>

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

struct ComplexIOR;

class Material
{
public:
    Material()
    {
        roughness = 0.0;
        specular_roughness = 0.0;
        ior = -1.0;
        complex_ior = nullptr;
        transparency = 0.0;
        perfect_mirror = false;
        reflectance = glm::dvec3(0.0);
        specular_reflectance = glm::dvec3(1.0);
        emittance = glm::dvec3(0.0);
        transmittance = glm::dvec3(1.0);

        computeProperties();
    }

    glm::dvec3 DiffuseBRDF(const glm::dvec3 &i, const glm::dvec3 &o);
    glm::dvec3 SpecularBRDF(const glm::dvec3 &i, const glm::dvec3 &o, bool exit_object = false);
    glm::dvec3 LambertianBRDF();
    glm::dvec3 OrenNayarBRDF(const glm::dvec3 &i, const glm::dvec3 &o);
    double GGXFactor(double cos_i, double cos_o);

    glm::dvec3 specularMicrofacetNormal(const glm::dvec3 &out) const;

    void computeProperties();

    glm::dvec3 reflectance, specular_reflectance, transmittance, emittance;
    double roughness, specular_roughness, ior, transparency, reflect_probability;
    std::shared_ptr<ComplexIOR> complex_ior;

    bool can_diffusely_reflect, rough, rough_specular;

    // Represents ior = infinity -> fresnel factor = 1.0 -> all rays specularly reflected
    bool perfect_mirror;

private:

    // Pre-computed Oren-Nayar variables.
    double A, B;
};

void from_json(const nlohmann::json &j, Material &m);

namespace std
{
    void from_json(const nlohmann::json &j, shared_ptr<Material> &m);
}