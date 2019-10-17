#include "Material.hpp"

glm::dvec3 Material::DiffuseBRDF(const glm::dvec3 &i, const glm::dvec3 &o)
{
    return roughness > 1e-7 ? OrenNayarBRDF(i, o) : LambertianBRDF();
}

glm::dvec3 Material::SpecularBRDF()
{
    return specular_reflectance;
}

glm::dvec3 Material::LambertianBRDF()
{
    return reflectance / M_PI;
}

glm::dvec3 Material::OrenNayarBRDF(const glm::dvec3 &i, const glm::dvec3 &o)
{
    // TODO: Check if compiler optimizes this, otherwise pre-compute in constructor
    const double variance = pow2(roughness);
    const double A = 1.0 - 0.5 * variance / (variance + 0.33);
    const double B = 0.45 * variance / (variance + 0.09);

    // equivalent to dot(normalize(i.x, i.y, 0), normalize(o.x, o.y, 0)), 
    // i.e. remove z-component (normal) and get the cos angle between vectors with dot
    double cos_delta_phi = glm::clamp((i.x*o.x + i.y*o.y) / sqrt((pow2(i.x) + pow2(i.y)) * (pow2(o.x) + pow2(o.y))), 0.0, 1.0);

    // C = sin(alpha) * tan(beta), i.z = dot(i, (0,0,1))
    double C = sqrt((1.0 - pow2(i.z)) * (1.0 - pow2(o.z))) / std::max(i.z, o.z);

    return (reflectance / M_PI) * (A + B * cos_delta_phi * C);
}

double Material::calculateReflectProbability()
{
    return (glm::compAdd(reflectance) + glm::compAdd(specular_reflectance)) / (20.0 / 3.0); // at most .9
}