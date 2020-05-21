#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

#include "util.hpp"

inline glm::dvec3 orthogonalUnitVector(const glm::dvec3& v)
{
    if (std::abs(v.x) > std::abs(v.y))
        return glm::dvec3(-v.z, 0, v.x) / std::sqrt(pow2(v.x) + pow2(v.z));
    else
        return glm::dvec3(0, v.z, -v.y) / std::sqrt(pow2(v.y) + pow2(v.z));
}

struct CoordinateSystem
{
    CoordinateSystem(const glm::dvec3& N) : normal(N)
    {
        glm::dvec3 X = orthogonalUnitVector(N);
        T = glm::dmat3(X, glm::cross(N, X), N);
    }

    glm::dvec3 localToGlobal(const glm::dvec3& v) const
    {
        return T * v;
    }

    glm::dvec3 globalToLocal(const glm::dvec3& v) const
    {
        return glm::transpose(T) * v;
    }

    static glm::dvec3 localToGlobal(const glm::dvec3& v, const glm::dvec3& N)
    {
        glm::dvec3 tX = orthogonalUnitVector(N);
        glm::dvec3 tY = glm::cross(N, tX);
        return tX * v.x + tY * v.y + N * v.z;
    }

    glm::dvec3 normal;

private:
    glm::dmat3 T;
};