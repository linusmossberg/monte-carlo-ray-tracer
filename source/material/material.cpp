#include "material.hpp"

#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>

#include "../common/util.hpp"
#include "../common/constants.hpp"
#include "../random/random.hpp"
#include "../common/coordinate-system.hpp"
#include "../camera/pixel-operators.hpp"
#include "../cie/cie.hpp"
#include "fresnel.hpp"

glm::dvec3 Material::DiffuseBRDF(const glm::dvec3 &i, const glm::dvec3 &o)
{
    return rough ? OrenNayarBRDF(i, o) : LambertianBRDF();
}

glm::dvec3 Material::SpecularBRDF(const glm::dvec3 &i, const glm::dvec3 &o)
{
    if (rough_specular) return GGXBRDF(i, o);

    return i.z < 0.0 ? transmittance : specular_reflectance;
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
    return (reflectance * C::INV_PI) * (A + B * cos_delta_phi * D);
}


glm::dvec3 Material::GGXBRDF(const glm::dvec3 &i, const glm::dvec3 &o)
{
    double a2 = pow2(specular_roughness);

    double cos_o = o.z;

    // Refracted rays has negative dot product with normal. Assuming that the statistical shadowing amount 
    // is the same on both sides, then just flipping the sign should be correct. It would probably 
    // have to be mirrored against the normal in the unidirectional anisotropic case as well.
    bool refraction = i.z < 0.0;
    double cos_i = refraction ? -i.z : i.z;
    
    // Slide 80:
    // https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf
    double denom_i = std::sqrt(a2 + (1.0 - a2) * pow2(cos_i));
    double denom_o = std::sqrt(a2 + (1.0 - a2) * pow2(cos_o));

    // G1 = 2.0 * cos_o / (denom_o + cos_o);
    // G2 = 2.0 * cos_i * cos_o / (cos_o * denom_i + cos_i * denom_o);
    double G2_div_G1 = (cos_i * (denom_o + cos_o)) / (cos_o * denom_i + cos_i * denom_o);

    return (refraction ? transmittance : specular_reflectance) * G2_div_G1;
}

// Samples a visible microfacet normal from the GGX distribution
// https://schuttejoe.github.io/post/ggximportancesamplingpart2/
// https://hal.archives-ouvertes.fr/hal-01509746/document
glm::dvec3 Material::specularMicrofacetNormal(const glm::dvec3 &out) const
{
    glm::dmat3 T;

    // Stretch the out-vector so we are sampling as if roughness is 1
    T[2] = glm::normalize(glm::dvec3(out.x * specular_roughness, out.y * specular_roughness, out.z));
    double out_z = T[2].z;

    // Choose a point on a disk with each half of the disk weighted
    // proportionally to its projection onto stretched out-direction
    double a = 1.0 / (1.0 + out_z);
    double r = std::sqrt(Random::range(0.0, 1.0));
    double u = Random::range(0.0, 1.0);
    double azimuth = ((u < a) ? (u / a) : 1.0 + (u - a) / (1.0 - a)) * C::PI;

    glm::dvec3 p;
    p.x = r * std::cos(azimuth);
    p.y = r * std::sin(azimuth) * ((u < a) ? 1.0 : out_z);
    p.z = std::sqrt(std::max(0.0, 1.0 - pow2(p.x) - pow2(p.y)));

    // Build orthonormal basis
    T[0] = (out_z < 1.0 - C::EPSILON) ? glm::normalize(glm::cross(T[2], glm::dvec3(0.0, 0.0, 1.0))) : glm::dvec3(1.0, 0.0, 0.0);
    T[1] = glm::cross(T[0], T[2]);

    // Calculate the normal in this stretched tangent space
    glm::dvec3 n = T * p;

    // Unstretch and normalize the normal
    return glm::normalize(glm::dvec3(specular_roughness * n.x, specular_roughness * n.y, std::max(0.0, n.z)));
}

void Material::computeProperties()
{
    can_diffusely_reflect = !complex_ior && !perfect_mirror && std::abs(transparency - 1.0) > C::EPSILON;

    rough = roughness > C::EPSILON;
    rough_specular = specular_roughness > C::EPSILON;

    double variance = pow2(roughness);
    A = 1.0 - 0.5 * (variance / (variance + 0.33));
    B = 0.45 * (variance / (variance + 0.09));

    // Use fresnel at 45 degree incidence and n1 in vacuum to determine Russian roulette reflect probability
    double cos_45 = std::cos(C::PI / 4.0);
    if (complex_ior)
    {
        reflect_probability = glm::compMax(specular_reflectance * Fresnel::conductor(1.0, complex_ior.get(), cos_45));
    }
    else
    {
        double R = Fresnel::dielectric(1.0, ior, cos_45);
        double T = transparency;

        double R_prob = glm::compMax((1.0 - T) * R * specular_reflectance);
        double T_prob = glm::compMax((1.0 - R) * T * transmittance);
        double D_prob = glm::compMax((1.0 - R) * (1.0 - T) * reflectance);

        reflect_probability = glm::max(R_prob, glm::max(T_prob, D_prob));
    }
    reflect_probability = std::min(reflect_probability, 0.8);
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

    m.reflectance = gammaExpand(m.reflectance);

    if (j.find("emittance") != j.end())
    {
        auto e = j.at("emittance");
        if (e.type() == nlohmann::json::value_t::object)
        {
            double scale = getOptional(e, "scale", 1.0);
            std::string illuminant = getOptional<std::string>(e, "illuminant", "D65");
            m.emittance = CIE::RGB(CIE::whitePoint(CIE::strToIlluminant(illuminant), scale));
        }
        else
        {
            m.emittance = e.get<glm::dvec3>();
        }
    }

    if (j.find("ior") != j.end())
    {
        auto type = j.at("ior").type();
        if(type == nlohmann::json::value_t::object)
        { 
            m.complex_ior = std::make_shared<ComplexIOR>();
            getToOptional(j.at("ior"), "real", m.complex_ior->real);
            getToOptional(j.at("ior"), "imaginary", m.complex_ior->imaginary);
        }
        else if (type == nlohmann::json::value_t::string)
        {
            auto path = std::filesystem::current_path() / "scenes";
            path /= j.at("ior").get<std::string>();

            if (std::filesystem::exists(path))
            {
                std::set<CIE::SpectralValue> real, imaginary;
                char type = 'n';

                std::ifstream file(path);
                std::string line;
                while (std::getline(file, line))
                {
                    auto p = line.find(',');
                    if (p != std::string::npos)
                    {
                        auto wl = line.substr(0, p);
                        auto v = line.substr(p + 1);
                        wl.erase(std::remove(wl.begin(), wl.end(), ' '), wl.end());
                        v.erase(std::remove(v.begin(), v.end(), ' '), v.end());

                        if (wl == "wl")
                        {
                            if (v == "n") type = 'n';
                            if (v == "k") type = 'k';
                        }
                        else
                        {
                            double wavelength = std::stod(wl) * 1e3; // micro- to nanometers
                            double value = std::stod(v);
                            if (type == 'n') real.insert({ wavelength, value });
                            if (type == 'k') imaginary.insert({ wavelength, value });
                        }
                    }
                }

                auto pad = [](std::set<CIE::SpectralValue> &d)
                {
                    if (d.empty()) d.insert({ CIE::MIN_WAVELENGTH, 0.0 });

                    if (d.begin()->wavelength > CIE::MIN_WAVELENGTH)
                        d.insert({ CIE::MIN_WAVELENGTH, d.begin()->value });

                    if ((--d.end())->wavelength < CIE::MAX_WAVELENGTH)
                        d.insert({ CIE::MAX_WAVELENGTH, (--d.end())->value });
                };

                pad(real);
                pad(imaginary);

                m.complex_ior = std::make_shared<ComplexIOR>();
                m.complex_ior->real = CIE::RGB(real);
                m.complex_ior->imaginary = CIE::RGB(imaginary);
            }
            else
            {
                std::cout << std::endl << path.string() << " not found.\n";
            }
        }
        else
        {
            j.at("ior").get_to(m.ior);
        }
    }

    m.computeProperties();
}

void std::from_json(const nlohmann::json &j, std::shared_ptr<Material> &m)
{
    m = std::make_shared<Material>(j.get<Material>());
}