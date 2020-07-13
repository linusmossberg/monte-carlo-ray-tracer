#include "coordinate-system.hpp"
#include "util.hpp"
#include "constants.hpp"

glm::dvec3 orthogonalUnitVector(const glm::dvec3& v)
{
    if (std::abs(v.x) > std::abs(v.y))
        return glm::dvec3(-v.z, 0, v.x) / std::sqrt(pow2(v.x) + pow2(v.z));
    else
        return glm::dvec3(0, v.z, -v.y) / std::sqrt(pow2(v.y) + pow2(v.z));
}

CoordinateSystem::CoordinateSystem(const glm::dvec3& N) : normal(N)
{
    glm::dvec3 X = orthogonalUnitVector(N);
    T = glm::dmat3(X, glm::cross(N, X), N);
}

glm::dvec3 CoordinateSystem::from(const glm::dvec3& v) const
{
    return T * v;
}

glm::dvec3 CoordinateSystem::to(const glm::dvec3& v) const
{
    return glm::transpose(T) * v;
}

glm::dvec3 CoordinateSystem::from(const glm::dvec3& v, const glm::dvec3& N)
{
    glm::dvec3 tX = orthogonalUnitVector(N);
    glm::dvec3 tY = glm::cross(N, tX);
    return tX * v.x + tY * v.y + N * v.z;
}