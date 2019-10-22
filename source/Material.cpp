#include "Material.hpp"

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

glm::dvec3 Material::OrenNayarBRDF(const glm::dvec3 &i, const glm::dvec3 &o)
{
    // TODO: Check if compiler optimizes this, otherwise pre-compute in constructor
    const double variance = pow2(roughness);
    const double A = 1.0 - 0.5 * variance / (variance + 0.33);
    const double B = 0.45 * variance / (variance + 0.09);

    // equivalent to dot(normalize(i.x, i.y, 0), normalize(o.x, o.y, 0)), 
    // i.e. remove z-component (normal) and get the cos angle between vectors with dot
    double cos_delta_phi = glm::clamp((i.x*o.x + i.y*o.y) / sqrt((pow2(i.x) + pow2(i.y)) * (pow2(o.x) + pow2(o.y))), 0.0, 1.0);

    // D = sin(alpha) * tan(beta), i.z = dot(i, (0,0,1))
    double D = sqrt((1.0 - pow2(i.z)) * (1.0 - pow2(o.z))) / std::max(i.z, o.z);

    return (reflectance / C::PI) * (A + B * cos_delta_phi * D);
}

// Schlick's approximation of fresnel factor
double Material::Fresnel(double n1, double n2, const glm::dvec3& normal, const glm::dvec3& dir)
{
    if (abs(n1 - n2) < C::EPSILON)
        return 0;

    double R0 = pow2((n1 - n2) / (n1 + n2));
    return R0 + (1.0 - R0) * pow(1.0 - glm::dot(normal, dir), 5);
}

double Material::calculateReflectProbability(double scene_ior)
{
    // Give max value to materials that can produce caustics
    if (perfect_mirror || std::abs(ior - scene_ior) > C::EPSILON) return 0.9;

    return (glm::compAdd(reflectance) + glm::compAdd(specular_reflectance)) / (20.0 / 3.0); // at most .9
}