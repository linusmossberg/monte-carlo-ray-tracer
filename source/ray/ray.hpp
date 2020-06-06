#pragma once

#include <glm/vec3.hpp>

#include "../common/coordinate-system.hpp"

class Ray
{
public:
    Ray();
    Ray(const glm::dvec3& start);
    Ray(const glm::dvec3& start, const glm::dvec3& end, double medium_ior = 1.0);

    glm::dvec3 operator()(double t) const;

    void reflectDiffuse(const CoordinateSystem& cs, double n1);
    void reflectSpecular(const glm::dvec3 &in, const glm::dvec3 &normal, double n1);
    void refractSpecular(const glm::dvec3 &in, const glm::dvec3 &normal, double n1, double n2);
    
    // Normalized direction -> t corresponds to euclidian distance in metric units
    glm::dvec3 start, direction;
    double medium_ior;
    bool specular = false;
};