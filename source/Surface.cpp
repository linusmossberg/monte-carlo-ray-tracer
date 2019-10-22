#include "Surface.hpp"

#include <iostream>

bool Surface::Sphere::intersect(const Ray& ray, Intersection& intersection) const 
{
    glm::dvec3 so = ray.start - origin;
    double b = glm::dot(ray.direction, so);
    double c = glm::dot(so, so) - pow2(radius);

    double discriminant = pow2(b) - c;
    if (discriminant < 0)
    {
        return false;
    }

    double v = std::sqrt(discriminant);
    double t = -b - v;
    if (t < 0)
    {
        t = v - b;
        if (t < 0)
        {
            return false;
        }
    }
    
    intersection.t = t;
    intersection.position = ray(t);
    intersection.normal = (intersection.position - origin) / radius;
    intersection.material = material;

    return true;
}

bool Surface::Triangle::intersect(const Ray& ray, Intersection& intersection) const
{
    glm::dvec3 P = glm::cross(ray.direction, E2);
    double denominator = glm::dot(P, E1);
    if (abs(denominator) < C::EPSILON) // Ray parallel to triangle. 
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

    intersection.t = t;
    intersection.position = ray(t);
    intersection.normal = normal_;
    intersection.material = material;

    return true;
}