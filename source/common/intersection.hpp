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

    size_t selectNewPath(double n1, double n2, const glm::dvec3& directon) const
    {
        double R = material->Fresnel(n1, n2, normal, directon);
        double T = material->transparency;

        double p = Random::range(0, 1);

        if (R > p)
        {
            return Path::REFLECT;
        }
        else if (R + (1 - R) * T > p)
        {
            return Path::REFRACT;
        }
        else // R + (1 - R) * T + (1 - R) * (1 - T) = 1 > p
        {
            return Path::DIFFUSE;
        }
    }

    explicit operator bool() const
    {
        return material.operator bool();
    }
};
