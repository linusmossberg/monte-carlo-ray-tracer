#pragma once

#include <glm/vec3.hpp>

struct BoundingBox
{
    BoundingBox() : min(0.0), max(0.0) { }
    BoundingBox(const glm::dvec3 min, const glm::dvec3 max) : min(min), max(max) { }

    glm::dvec3 min, max;
};
