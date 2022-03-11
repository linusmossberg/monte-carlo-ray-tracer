#include "coordinate-system.hpp"

#include <cmath>

// Building an Orthonormal Basis, Revisited. - Duff et al.
// https://graphics.pixar.com/library/OrthonormalB/paper.pdf
glm::dmat3 orthonormalBasis(const glm::dvec3& N)
{
    double sign = std::copysign(1.0, N.z);
    double a = -1.0 / (sign + N.z);
    double b = N.x * N.y * a;
    return
    {
        { 1.0 + sign * N.x * N.x * a, sign * b, -sign * N.x },
        { b, sign + N.y * N.y * a, -N.y },
        N
    };
}

CoordinateSystem::CoordinateSystem(const glm::dvec3& N) : T(orthonormalBasis(N)) { }

glm::dvec3 CoordinateSystem::from(const glm::dvec3& v) const
{
    return T * v;
}

glm::dvec3 CoordinateSystem::to(const glm::dvec3& v) const
{
    return glm::transpose(T) * v;
}

const glm::dvec3& CoordinateSystem::normal() const
{
    return T[2];
}

glm::dvec3 CoordinateSystem::from(const glm::dvec3& v, const glm::dvec3& N)
{
    return orthonormalBasis(N) * v;
}