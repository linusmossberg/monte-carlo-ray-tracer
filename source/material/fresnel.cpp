#include "fresnel.hpp"

#include <fstream>
#include <iostream>

#include <glm/glm.hpp>

#include "../common/constexpr-math.hpp"
#include "../common/constants.hpp"
#include "../common/util.hpp"
#include "../scene/scene.hpp"
#include "../color/spectral.hpp"
#include "../color/srgb.hpp"

// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
double Fresnel::dielectric(double n1, double n2, double cos_theta)
{
    if (n2 < 1.0 || std::abs(n1 - n2) < C::EPSILON) return 0.0;

    double sqr_g = pow2(n2 / n1) + pow2(cos_theta) - 1.0;

    if (sqr_g < 0.0) return 1.0;

    double g = std::sqrt(sqr_g);
    double g_p_c = g + cos_theta;
    double g_m_c = g - cos_theta;

    return 0.5 * pow2(g_m_c / g_p_c) * (1.0 + pow2((g_p_c * cos_theta - 1.0) / (g_m_c * cos_theta + 1.0)));
}

// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
glm::dvec3 Fresnel::conductor(double n1, ComplexIOR* n2, double cos_theta)
{
    double cos_theta2 = pow2(cos_theta);
    double sin_theta2 = 1.0 - cos_theta2;

    glm::dvec3 eta2 = pow2(n2->real / n1);
    glm::dvec3 eta_k2 = pow2(n2->imaginary / n1);

    glm::dvec3 t0 = eta2 - eta_k2 - sin_theta2;
    glm::dvec3 a2_p_b2 = glm::sqrt(pow2(t0) + 4.0 * eta2 * eta_k2);
    glm::dvec3 t1 = a2_p_b2 + cos_theta2;
    glm::dvec3 t2 = 2.0 * cos_theta * glm::sqrt(0.5 * (a2_p_b2 + t0));
    glm::dvec3 r_perpendicual = (t1 - t2) / (t1 + t2); 

    glm::dvec3 t3 = cos_theta2 * a2_p_b2 + pow2(sin_theta2);
    glm::dvec3 t4 = t2 * sin_theta2;
    glm::dvec3 r_parallel = r_perpendicual * (t3 - t4) / (t3 + t4); 

    return (r_parallel + r_perpendicual) * 0.5;
}

void from_json(const nlohmann::json &j, ComplexIOR &c)
{
    if (j.type() == nlohmann::json::value_t::object)
    {
        c.real = getOptional(j, "real", glm::dvec3(1.0));
        c.imaginary = getOptional(j, "imaginary", glm::dvec3(0.0));
    }
    else if (j.type() == nlohmann::json::value_t::string)
    {
        auto ior_path = Scene::path / j.get<std::string>();

        if (std::filesystem::exists(ior_path))
        {
            Spectral::Distribution<double> real, imaginary;
            char type = 'n';

            std::ifstream file(ior_path);
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

            c.real = sRGB::RGB(real, Spectral::REFLECTANCE);
            c.imaginary = sRGB::RGB(imaginary, Spectral::REFLECTANCE);
        }
        else
        {
            std::cout << std::endl << ior_path.string() << " not found.\n";
        }
    }
}