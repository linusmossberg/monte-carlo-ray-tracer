#pragma once

#include <glm/glm.hpp>

namespace GGX
{
    double D(const glm::dvec3& m, const glm::dvec2& a);
    double DV(const glm::dvec3& m, const glm::dvec3& wo, const glm::dvec2& a);
    double Lambda(const glm::dvec3& wo, const glm::dvec2& a);
    double SmithG1(const glm::dvec3& wo, const glm::dvec2& a);
    double SmithG2(const glm::dvec3& wi, const glm::dvec3& wo, const glm::dvec2& a);
    double reflection(const glm::dvec3& wi, const glm::dvec3& wo, const glm::dvec2& a, double& PDF);
    double transmission(const glm::dvec3& wi, const glm::dvec3& wo, double n1, double n2, const glm::dvec2& a, double& PDF);
    glm::dvec3 visibleMicrofacet(double u, double v, const glm::dvec3& wo, const glm::dvec2& a);
}