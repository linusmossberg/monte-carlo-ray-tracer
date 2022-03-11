#pragma once

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

struct CoordinateSystem
{
    CoordinateSystem() : T() { }
    CoordinateSystem(const glm::dvec3& N);

    glm::dvec3 from(const glm::dvec3& v) const;
    glm::dvec3 to(const glm::dvec3& v) const;

    static glm::dvec3 from(const glm::dvec3& v, const glm::dvec3& N);

    const glm::dvec3& normal() const;

private:
    glm::dmat3 T;
};