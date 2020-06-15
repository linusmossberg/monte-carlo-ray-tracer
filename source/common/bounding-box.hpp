#pragma once

#include <glm/vec3.hpp>
#include "../ray/ray.hpp"

struct BoundingBox
{
    BoundingBox() : centroid((max + min) / 2.0) { }
    BoundingBox(const glm::dvec3 min, const glm::dvec3 max) 
        : min(min), max(max), centroid((max + min) / 2.0) { }

    void computeProperties();
    bool intersect(const Ray &ray, double &t) const;
    bool contains(const glm::dvec3 &p) const;
    glm::dvec3 dimensions() const;
    double area() const;
    void merge(const BoundingBox &BB);
    void merge(const glm::dvec3 &p);

    glm::dvec3 min = glm::dvec3(std::numeric_limits<double>::max());
    glm::dvec3 max = glm::dvec3(std::numeric_limits<double>::lowest());
    glm::dvec3 centroid;
};
