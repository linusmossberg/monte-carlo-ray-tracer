#pragma once

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

glm::dvec3 orthogonalUnitVector(const glm::dvec3& v);

struct CoordinateSystem
{
    CoordinateSystem(const glm::dvec3& N);

    glm::dvec3 localToGlobal(const glm::dvec3& v) const;

    glm::dvec3 globalToLocal(const glm::dvec3& v) const;

    static glm::dvec3 localToGlobal(const glm::dvec3& v, const glm::dvec3& N);

    glm::dvec3 normal;

private:
    glm::dmat3 T;
};