#pragma once

#include <glm/vec3.hpp>
#include "../ray/ray.hpp"

struct BoundingBox
{
    BoundingBox() : min(0.0), max(0.0) { }
    BoundingBox(const glm::dvec3 min, const glm::dvec3 max) : min(min), max(max) { }

    bool intersect(const Ray &ray, double &t) const;
    bool contains(const glm::dvec3 &p) const;

    glm::dvec3 min, max;
};
