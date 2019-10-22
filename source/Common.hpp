
/**********************************************************
Common types and structures that are used in several places 
but are too small to warrant having their own files.
***********************************************************/

#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>

#include "Material.hpp"
#include "Util.hpp"

struct CoordinateSystem
{
    CoordinateSystem(const glm::dvec3& N) : normal(N)
    {
        glm::dvec3 X = orthogonalUnitVector(N);
        T = glm::dmat3(X, glm::cross(N, X), N);
    }

    glm::dvec3 localToGlobal(const glm::dvec3& v) const
    {
        return glm::normalize(T * v);
    }

    glm::dvec3 globalToLocal(const glm::dvec3& v) const
    {
        return glm::normalize(glm::inverse(T) * v);
    }

    static glm::dvec3 localToGlobalUnitVector(const glm::dvec3& v, const glm::dvec3& N)
    {
        glm::dvec3 tX = orthogonalUnitVector(N);
        glm::dvec3 tY = glm::cross(N, tX);

        return glm::normalize(tX * v.x + tY * v.y + N * v.z);
    }

    glm::dvec3 normal;

private:
    glm::dmat3 T;
};

struct Intersection
{
    glm::dvec3 position;
    glm::dvec3 normal;
    double t = std::numeric_limits<double>::max();
    std::shared_ptr<Material> material;

    explicit operator bool() const
    {
        return t != std::numeric_limits<double>::max();
    }
};

struct BoundingBox
{
    BoundingBox() : min(0.0), max(0.0) { }
    BoundingBox(const glm::dvec3 min, const glm::dvec3 max) : min(min), max(max) { }

    glm::dvec3 min, max;
};
