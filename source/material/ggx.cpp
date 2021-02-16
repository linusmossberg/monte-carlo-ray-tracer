#include "ggx.hpp"

#include "../common/constexpr-math.hpp"
#include "../common/constants.hpp"
#include "../random/random.hpp"

/*

- Sampling the GGX Distribution of Visible Normals
  http://jcgt.org/published/0007/04/01/

- Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs
  http://jcgt.org/published/0003/02/03/

- Microfacet Models for Refraction through Rough Surfaces
  https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html

*/

double GGX::D(const glm::dvec3& m, const glm::dvec2& a)
{
    return 1.0 / (C::PI * a.x * a.y * pow2(pow2(m.x / a.x) + pow2(m.y / a.y) + pow2(m.z)));
}

double GGX::DV(const glm::dvec3& m, const glm::dvec3& wo, const glm::dvec2& a)
{
    return SmithG1(wo, a) * glm::dot(wo, m) * D(m, a) / wo.z;
}

double GGX::Lambda(const glm::dvec3& wo, const glm::dvec2& a)
{
    return (-1.0 + std::sqrt(1.0 + (pow2(a.x * wo.x) + pow2(a.y * wo.y)) / (pow2(wo.z)))) / 2.0;
}

double GGX::SmithG1(const glm::dvec3& wo, const glm::dvec2& a)
{
    return 1.0 / (1.0 + Lambda(wo, a));
}

double GGX::SmithG2(const glm::dvec3& wi, const glm::dvec3& wo, const glm::dvec2& a)
{
    return 1.0 / (1.0 + Lambda(wo, a) + Lambda(wi, a));
}

double GGX::reflection(const glm::dvec3& wi, const glm::dvec3& wo, const glm::dvec2& a, double& PDF)
{
    glm::dvec3 m = glm::normalize(wo + wi);

    PDF = DV(m, wo, a) / (4.0 * glm::dot(m, wo));
    return  D(m, a) * SmithG2(wi, wo, a) / (4.0 * wo.z * wi.z);
}

double GGX::transmission(const glm::dvec3& wi, const glm::dvec3& wo, double n1, double n2, const glm::dvec2& a, double& PDF)
{
    glm::dvec3 m = wo * n1 + wi * n2;
    double m_length2 = glm::dot(m, m);
    m /= std::sqrt(m_length2);
    if (n1 < n2) m = -m;

    double dm_dwi = pow2(n2) * std::abs(glm::dot(wi, m)) / m_length2;

    PDF = DV(m, wo, a) * dm_dwi;
    return std::abs(SmithG2(wi, wo, a) * D(m, a) * glm::dot(wo, m) * dm_dwi / (wo.z * wi.z));
}

glm::dvec3 GGX::visibleMicrofacet(const glm::dvec3& wo, const glm::dvec2& a)
{
    glm::dvec3 Vh = glm::normalize(glm::dvec3(a.x * wo.x, a.y * wo.y, wo.z));

    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    double len2 = pow2(Vh.x) + pow2(Vh.y);
    glm::dvec3 T1 = len2 > 0.0 ? glm::dvec3(-Vh.y, Vh.x, 0.0) * glm::inversesqrt(len2) : glm::dvec3(1.0, 0.0, 0.0);
    glm::dvec3 T2 = glm::cross(Vh, T1);

    // Section 4.2: parameterization of the projected area
    double r = std::sqrt(Random::unit());
    double phi = Random::angle();
    double t1 = r * std::cos(phi);
    double t2 = r * std::sin(phi);
    double s = 0.5 * (1.0 + Vh.z);
    t2 = (1.0 - s) * std::sqrt(1.0 - pow2(t1)) + s * t2;

    // Section 4.3: reprojection onto hemisphere
    glm::dvec3 Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.0, 1.0 - pow2(t1) - pow2(t2))) * Vh;

    // Section 3.4: transforming the normal back to the ellipsoid configuration
    return glm::normalize(glm::dvec3(a.x * Nh.x, a.y * Nh.y, std::max(0.0, Nh.z)));
}