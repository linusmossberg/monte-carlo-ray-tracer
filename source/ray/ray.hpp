#pragma once

#include <glm/vec3.hpp>
#include <vector>

#include "../common/coordinate-system.hpp"

struct Interaction;

class Ray
{
public:
    Ray(const Interaction& ia);
    Ray(const glm::dvec3& start, const glm::dvec3& end);
    Ray(const glm::dvec3& start, const glm::dvec3& direction, double medium_ior);

    glm::dvec3 operator()(double t) const;

    glm::dvec3 start, direction, inv_direction;
    double medium_ior;
    double refraction_scale = 1.0;
    bool dirac_delta = false, refraction = false;
    uint16_t depth = 0, diffuse_depth = 0;

    int refraction_level = 0;
};

struct RefractionHistory
{
    RefractionHistory(const Ray& ray);
    void update(const Ray& ray);
    double externalIOR(const Ray& ray) const;

private:
    std::vector<double> iors;
};