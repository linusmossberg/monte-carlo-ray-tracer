#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Material.hpp"
#include "Random.hpp"
#include "CoordinateSystem.hpp"

class Ray
{
public:
    Ray()
        : start(0), direction(0, 0, 1), medium_ior(1.0) { }

    Ray(const glm::dvec3& start, const glm::dvec3& end, double medium_ior = 1.0)
        : start(start), direction(glm::normalize(end - start)), medium_ior(medium_ior) { }

    Ray(const glm::dvec3& start)
        : start(start), direction(0,0,1), medium_ior(1.0) { }

    glm::dvec3 operator()(double t) const
    {
        return start + direction * t;
    }

    void reflectDiffuse(const CoordinateSystem& cs, double n1);
    void reflectSpecular(const glm::dvec3 &in, const glm::dvec3 &normal, double n1);
    void refractSpecular(const glm::dvec3 &in, const glm::dvec3 &normal, double n1, double n2);
    
    // Normalized direction -> t corresponds to euclidian distance in metric units
    glm::dvec3 start, direction;
    double medium_ior;
    const double offset = C::EPSILON;
    bool specular = false;
};