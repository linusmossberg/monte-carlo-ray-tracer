#include "Surface.h"

#include <iostream>

bool Surface::Sphere::intersect(const Ray& ray, Intersection& intersection) const 
{
	float b = glm::dot(2.0f * ray.direction, ray.start - origin);
	float c = glm::dot(ray.start - origin, ray.start - origin) - radius * radius;

	float t = -b / 2 - sqrt(pow(b / 2, 2) - c);
	t = t > 0 ? t : -b / 2 + sqrt(pow(b / 2, 2) - c);

	if (isnan(t) || t < 0)
	{
		return false;
	}

	intersection.t = t;
	intersection.position = ray(t);
	intersection.normal = (intersection.position - origin) / radius;

	return true;
}

bool Surface::Triangle::intersect(const Ray& ray, Intersection& intersection) const
{
	glm::vec3 P = glm::cross(ray.direction, E2);
	float denominator = glm::dot(P, E1);
	if (abs(denominator) < 0.0000001) // Ray parallel to triangle. 
	{
		return false; // The other checks will catch this but doing it here reduces computations.
	}

	glm::vec3 T = ray.start - v0;
	float u = glm::dot(P, T) / denominator;
	if (u > 1.0f || u < 0.0f)
	{
		return false;
	}

	glm::vec3 Q = glm::cross(T, E1);
	float v = glm::dot(Q, ray.direction) / denominator;
	if (v > 1.0f || v < 0.0f || u + v > 1.0f)
	{
		return false;
	}

	float t = glm::dot(Q, E2) / denominator;
	if (t <= 0.0f)
	{
		return false;
	}

	intersection.t = t;
	intersection.position = ray(t);
	intersection.normal = normal;

	return true;
}