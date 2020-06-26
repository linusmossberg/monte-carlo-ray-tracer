#pragma once

#include <glm/vec3.hpp>
#include "../ray/ray.hpp"

struct BoundingBox
{
    BoundingBox() { }
    BoundingBox(const glm::dvec3 min, const glm::dvec3 max) 
        : min(min), max(max) { }

    bool intersect(const Ray &ray, double &t) const;
    bool contains(const glm::dvec3 &p) const;
    glm::dvec3 dimensions() const;
    glm::dvec3 centroid() const;
    double area() const;
    double distance2(const glm::dvec3 &p) const;
    void merge(const BoundingBox &BB);
    void merge(const glm::dvec3 &p);
    bool valid() const;

    glm::dvec3 min = glm::dvec3(std::numeric_limits<double>::max());
    glm::dvec3 max = glm::dvec3(std::numeric_limits<double>::lowest());
};
