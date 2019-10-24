#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Material.hpp"

struct Intersection
{
    glm::dvec3 position;
    glm::dvec3 normal;
    double t = std::numeric_limits<double>::max();
    std::shared_ptr<Material> material;

    explicit operator bool() const
    {
        return material.operator bool();
    }
};
