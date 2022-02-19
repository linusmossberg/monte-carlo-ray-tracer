#pragma once

#include <memory>

#include <glm/vec3.hpp>

#include "ray.hpp"
#include "intersection.hpp"

class Material;
namespace Surface { class Base; };

struct Interaction
{
    Interaction(const Intersection &intersection, const Ray &ray, double external_ior);

    enum Type
    {
        REFLECT,
        REFRACT,
        DIFFUSE
    };

    Type type;

    bool sampleBSDF(glm::dvec3& bsdf_absIdotN, double& pdf, Ray& new_ray, bool flux = false) const;
    bool BSDF(glm::dvec3& bsdf_absIdotN, const glm::dvec3& world_wi, double& pdf) const;

    glm::dvec3 specularNormal() const;
    
    // n1 and n2 are correctly ordered.
    double t, n1, n2, T, R;
    std::shared_ptr<Material> material;
    std::shared_ptr<Surface::Base> surface;
    glm::dvec3 position, normal, out;
    CoordinateSystem shading_cs;
    bool inside, dirac_delta;
    Ray ray;

private:
    glm::dvec3 BSDF(const glm::dvec3& wo, const glm::dvec3& wi, double& pdf, bool flux, bool wi_dirac_delta) const;
    void selectType();
};
