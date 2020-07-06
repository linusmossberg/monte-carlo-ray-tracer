#pragma once

#include <memory>

#include <glm/vec3.hpp>

#include "ray.hpp"
#include "intersection.hpp"

class Material;

struct Interaction
{
    Interaction() {}

    Interaction(const Intersection &intersection, const Ray &ray, double environment_ior);

    enum Type
    {
        REFLECT,
        REFRACT,
        DIFFUSE
    };

    Type type;

    glm::dvec3 BRDF(const glm::dvec3 &in) const;
    
    double t, n1, n2;
    std::shared_ptr<Material> material;
    glm::dvec3 position, normal, out;
    CoordinateSystem cs;

    Ray getNewRay() const;

private:
    void selectType(const glm::dvec3 &specular_normal);
};
