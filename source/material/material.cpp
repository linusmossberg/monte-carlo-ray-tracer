#include "material.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>

#include "../common/util.hpp"
#include "../common/constants.hpp"

glm::dvec3 Material::DiffuseBRDF(const glm::dvec3 &i, const glm::dvec3 &o)
{
    return roughness > C::EPSILON ? OrenNayarBRDF(i, o) : LambertianBRDF();
}

glm::dvec3 Material::SpecularBRDF()
{
    return specular_reflectance;
}

glm::dvec3 Material::LambertianBRDF()
{
    return reflectance / C::PI;
}

// Avoids trigonometric functions for increased performance.
glm::dvec3 Material::OrenNayarBRDF(const glm::dvec3 &i, const glm::dvec3 &o)
{
    // equivalent to dot(normalize(i.x, i.y, 0), normalize(o.x, o.y, 0)).
    // i.e. remove z-component (normal) and get the cos angle between vectors with dot
    double cos_delta_phi = glm::clamp((i.x*o.x + i.y*o.y) / std::sqrt((pow2(i.x) + pow2(i.y)) * (pow2(o.x) + pow2(o.y))), 0.0, 1.0);

    // D = sin(alpha) * tan(beta), i.z = dot(i, (0,0,1))
    double D = std::sqrt((1.0 - pow2(i.z)) * (1.0 - pow2(o.z))) / std::max(i.z, o.z);

    // A and B are pre-computed in constructor.
    return (reflectance / C::PI) * (A + B * cos_delta_phi * D);
}

// Schlick's approximation of fresnel factor
double Material::Fresnel(double n1, double n2, const glm::dvec3& normal, const glm::dvec3& dir) const
{
    if (perfect_mirror) return 1;

    if (std::abs(n1 - n2) < C::EPSILON || n2 < 1.0) return 0;

    double R0 = pow2((n1 - n2) / (n1 + n2));
    return R0 + (1.0 - R0) * std::pow(1.0 - glm::dot(normal, dir), 5);
}

void Material::computeProperties()
{
    can_diffusely_reflect = !perfect_mirror && std::abs(transparency - 1.0) > C::EPSILON;
    reflect_probability = calculateReflectProbability();

    double variance = pow2(roughness);
    A = 1.0 - 0.5 * variance / (variance + 0.33);
    B = 0.45 * variance / (variance + 0.09);
}

double Material::calculateReflectProbability()
{
    return std::min(std::max(glm::compMax(reflectance), glm::compMax(specular_reflectance)), 0.8);
}

void from_json(const nlohmann::json &j, Material &m)
{
    getToOptional(j, "roughness", m.roughness);
    getToOptional(j, "ior", m.ior);
    getToOptional(j, "transparency", m.transparency);
    getToOptional(j, "perfect_mirror", m.perfect_mirror);
    getToOptional(j, "reflectance", m.reflectance);
    getToOptional(j, "specular_reflectance", m.specular_reflectance);
    getToOptional(j, "emittance", m.emittance);

    m.computeProperties();
}

void std::from_json(const nlohmann::json &j, std::shared_ptr<Material> &m)
{
    m = std::make_shared<Material>(j.get<Material>());
}