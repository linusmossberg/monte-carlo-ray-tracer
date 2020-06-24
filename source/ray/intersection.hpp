#pragma once

#include <memory>

#include <glm/vec3.hpp>

#include "../material/material.hpp"

struct Intersection
{
    Intersection() : position(), normal() { }

    Intersection(const glm::dvec3 p, const glm::dvec3 n, double t, std::shared_ptr<Material> m)
        : position(p), normal(n), t(t), material(m) { }

    size_t selectNewPath(double n1, double n2, const glm::dvec3& direction) const;

    const glm::dvec3& shadingNormal() const 
    { 
        return use_interpolated ? interpolated_normal : normal;
    }

    glm::dvec3 position;
    glm::dvec3 normal;
    double t = std::numeric_limits<double>::max();
    std::shared_ptr<Material> material;

    glm::dvec3 interpolated_normal;
    bool use_interpolated = false;

    explicit operator bool() const
    {
        return material.operator bool();
    }
};
