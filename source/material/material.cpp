#include "material.hpp"

#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>

#include "../common/util.hpp"
#include "../common/constexpr-math.hpp"
#include "../common/constants.hpp"
#include "../common/coordinate-system.hpp"
#include "../color/illuminant.hpp"
#include "../color/srgb.hpp"
#include "fresnel.hpp"
#include "ggx.hpp"

glm::dvec3 Material::diffuseReflection(const glm::dvec3& wi, const glm::dvec3& wo, double& PDF) const
{
    if (wi.z < 0.0)
    {
        PDF = 0.0;
        return glm::dvec3(0.0);
    }

    PDF = wi.z * C::INV_PI;
    return rough ? OrenNayar(wi, wo) : lambertian();
}

glm::dvec3 Material::specularReflection(const glm::dvec3& wi, const glm::dvec3& wo, double& PDF) const
{
    if (wi.z < 0.0)
    {
        PDF = 0.0;
        return glm::dvec3(0.0);
    }
    if (rough_specular)
    {
        return specular_reflectance * GGX::reflection(wi, wo, a, PDF);
    }
    else
    {
        PDF = 1.0;
        return specular_reflectance / std::abs(wi.z);
    }        
}

glm::dvec3 Material::specularTransmission(const glm::dvec3& wi, const glm::dvec3& wo, double n1, 
                                          double n2, double& PDF, bool inside, bool flux) const
{
    if (wi.z > 0.0)
    {
        PDF = 0.0;
        return glm::dvec3(0.0);
    }

    glm::dvec3 btdf = !inside ? transmittance : glm::dvec3(1.0);
    if (rough_specular)
    {
        btdf *= GGX::transmission(wi, wo, n1, n2, a, PDF);
        if (flux) btdf *= pow2(n2 / n1);
    }
    else
    {
        PDF = 1.0;
        btdf *= transmittance / std::abs(wi.z);
        if (!flux) btdf *= pow2(n1 / n2);
    }
    return btdf;
}

glm::dvec3 Material::visibleMicrofacet(double u, double v, const glm::dvec3& wo) const
{
    return GGX::visibleMicrofacet(u, v, wo, a);
}

glm::dvec3 Material::lambertian() const
{
    return reflectance * C::INV_PI;
}

// Avoids trigonometric functions for increased performance.
glm::dvec3 Material::OrenNayar(const glm::dvec3& wi, const glm::dvec3& wo) const
{
    // equivalent to dot(normalize(i.x, i.y, 0), normalize(o.x, o.y, 0)).
    // i.e. remove z-component (normal) and get the cos angle between vectors with dot
    double cos_delta_phi = glm::clamp((wi.x*wo.x + wi.y*wo.y) / 
                           std::sqrt((pow2(wi.x) + pow2(wi.y)) * 
                           (pow2(wo.x) + pow2(wo.y))), 0.0, 1.0);

    // D = sin(alpha) * tan(beta), i.z = dot(i, (0,0,1))
    double D = std::sqrt((1.0 - pow2(wi.z)) * (1.0 - pow2(wo.z))) / std::max(wi.z, wo.z);

    // A and B are pre-computed in constructor.
    return lambertian() * (A + B * cos_delta_phi * D);
}

void Material::computeProperties()
{
    rough = roughness > C::EPSILON;
    rough_specular = specular_roughness > C::EPSILON;
    opaque = transparency < C::EPSILON || complex_ior || perfect_mirror;
    emissive = glm::compMax(emittance) > C::EPSILON;

    dirac_delta = (complex_ior || perfect_mirror || (std::abs(transparency - 1.0) < C::EPSILON)) && !rough_specular;

    double variance = pow2(roughness);
    A = 1.0 - 0.5 * (variance / (variance + 0.33));
    B = 0.45 * (variance / (variance + 0.09));

    a = glm::dvec2(specular_roughness);
}

void from_json(const nlohmann::json &j, Material &m)
{
    auto getReflectance = [&](const std::string &field, glm::dvec3 &reflectance)
    {
        if (j.find(field) != j.end())
        {
            const nlohmann::json &r = j.at(field);
            if (r.type() == nlohmann::json::value_t::string)
            {
                std::string hex_string = r.get<std::string>();
                if (hex_string.size() == 7 && hex_string[0] == '#')
                {
                    hex_string.erase(0, 1);
                    std::stringstream ss;
                    ss << std::hex << hex_string;

                    uint32_t color_int;
                    ss >> color_int;

                    reflectance = intToColor(color_int);
                }
            }
            else
            {
                reflectance = r.get<glm::dvec3>();
            }
        }
    };

    getToOptional(j, "roughness", m.roughness);
    getToOptional(j, "specular_roughness", m.specular_roughness);
    getToOptional(j, "transparency", m.transparency);
    getToOptional(j, "perfect_mirror", m.perfect_mirror);
    getReflectance("reflectance", m.reflectance);
    getReflectance("specular_reflectance", m.specular_reflectance);
    getReflectance("transmittance", m.transmittance);

    m.reflectance = sRGB::gammaExpand(m.reflectance);

    if (j.find("emittance") != j.end())
    {
        auto e = j.at("emittance");
        if (e.type() == nlohmann::json::value_t::object)
        {
            double scale = getOptional(e, "scale", 1.0);
            double temperature = getOptional<double>(e, "temperature", -1.0);

            if (temperature > 0.0)
            {
                m.emittance = sRGB::RGB(CIE::Illuminant::blackbody(temperature) * scale);
            }
            else
            {
                std::string illuminant = getOptional<std::string>(e, "illuminant", "D65");
                std::transform(illuminant.begin(), illuminant.end(), illuminant.begin(), toupper);
                m.emittance = sRGB::RGB(CIE::Illuminant::whitePoint(illuminant.c_str()) * scale);
            }
        }
        else
        {
            m.emittance = e.get<glm::dvec3>();
        }
    }

    if (j.find("ior") != j.end())
    {
        auto i = j.at("ior");
        if (i.type() == nlohmann::json::value_t::object || i.type() == nlohmann::json::value_t::string)
        {
            m.complex_ior = std::make_shared<ComplexIOR>(i.get<ComplexIOR>());
        }
        else
        {
            i.get_to(m.ior);
        }
    }

    m.computeProperties();
}

void std::from_json(const nlohmann::json &j, std::shared_ptr<Material> &m)
{
    m = std::make_shared<Material>(j.get<Material>());
}