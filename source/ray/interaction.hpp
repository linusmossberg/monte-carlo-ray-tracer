#pragma once

#include <memory>

#include <glm/vec3.hpp>

class Material;

struct Interaction
{
    Interaction() { }

    enum Type
    {
        REFLECT,
        REFRACT,
        DIFFUSE
    };

    Type type(double n1, double n2, const glm::dvec3& direction) const;

    double t;
    std::shared_ptr<Material> material;
    glm::dvec3 position, normal, shading_normal;

    explicit operator bool() const
    {
        return material.operator bool();
    }
};
