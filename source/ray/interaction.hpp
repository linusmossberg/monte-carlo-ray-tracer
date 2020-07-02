#pragma once

#include <memory>

#include <glm/vec3.hpp>

#include "ray.hpp"
#include "intersection.hpp"

class Material;

struct Interaction
{
    Interaction() {}

    Interaction(const Intersection &intersection, const Ray &ray, bool limited = false);

    enum Type
    {
        REFLECT,
        REFRACT,
        DIFFUSE
    };

    Type type(double n1, double n2, const glm::dvec3& direction);

    double t;
    std::shared_ptr<Material> material;
    glm::dvec3 position, normal, shading_normal, specular_normal;

    explicit operator bool() const
    {
        return material.operator bool();
    }
};
