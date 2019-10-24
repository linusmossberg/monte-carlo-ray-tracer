#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "../material/material.hpp"

struct Intersection
{
    Intersection() : position(), normal() { }

    Intersection(const glm::dvec3 p, const glm::dvec3 n, double t, std::shared_ptr<Material> m)
        : position(p), normal(n), t(t), material(m) { }

    glm::dvec3 position;
    glm::dvec3 normal;
    double t = std::numeric_limits<double>::max();
    std::shared_ptr<Material> material;

    explicit operator bool() const
    {
        return material.operator bool();
    }
};
