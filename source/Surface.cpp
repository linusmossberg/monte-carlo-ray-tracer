#include "Surface.h"

#include <iostream>

bool Surface::Sphere::intersect(const Ray& ray, Intersection& intersection) const 
{
	double b = glm::dot(2.0 * ray.direction, ray.start - origin);
	double c = glm::dot(ray.start - origin, ray.start - origin) - radius * radius;

	double t = -b / 2 - sqrt(pow2(b / 2) - c);
	t = t > 0 ? t : -b / 2 + sqrt(pow2(b / 2) - c);

	if (isnan(t) || t < 0)
	{
		return false;
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
	if (abs(denominator) < 0.0000001) // Ray parallel to triangle. 
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
	intersection.normal = normal;
	intersection.material = material;

	return true;
}