#include "surface.hpp"

bool Surface::Triangle::intersect(const Ray& ray, Intersection& intersection) const
{
    glm::dvec3 P = glm::cross(ray.direction, E2);
    double denominator = glm::dot(P, E1);
    if (std::abs(denominator) < C::EPSILON) // Ray parallel to triangle. 
    {
        return false;
    }

    glm::dvec3 T = ray.start - v0;
    double u = glm::dot(P, T) / denominator;
    if (u > 1.0 || u < 0.0)
    {
        return false;
    }

    glm::dvec3 Q = glm::cross(T, E1);
    double v = glm::dot(Q, ray.direction) / denominator;
    if (v > 1.0 || v < 0.0 || u + v > 1.0)
    {
        return false;
    }

    double t = glm::dot(Q, E2) / denominator;
    if (t <= 0.0)
    {
        return false;
    }

    intersection = Intersection(ray(t), normal_, t, material);

    return true;
}

glm::dvec3 Surface::Triangle::operator()(double u, double v) const
{
    double su = std::sqrt(u);
    return (1 - su) * v0 + (1 - v) * su * v1 + v * su * v2;
}

glm::dvec3 Surface::Triangle::normal(const glm::dvec3& pos) const
{
    return normal_;
}

BoundingBox Surface::Triangle::boundingBox() const
{
    BoundingBox bb;
    for (uint8_t c = 0; c < 3; c++)
    {
        bb.min[c] = std::min(v0[c], std::min(v1[c], v2[c]));
        bb.max[c] = std::max(v0[c], std::max(v1[c], v2[c]));
    }
    return bb;
}

void Surface::Triangle::computeArea()
{
    area_ = glm::length(glm::cross(E1, E2)) / 2.0;
}