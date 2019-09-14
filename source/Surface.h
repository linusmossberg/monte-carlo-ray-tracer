#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Ray.h"

namespace Surface
{
	class Base
	{
	public:
		Base() {};
		virtual ~Base() {};
		virtual bool intersect(const Ray& ray, Intersection& intersection) const = 0;
	};

	class Sphere : public Base
	{
	public:
		Sphere(const glm::dvec3& origin, double radius)
			: origin(origin), radius(radius) { }

		~Sphere() {};

		virtual bool intersect(const Ray& ray, Intersection& intersection) const;

	private:
		glm::dvec3 origin;
		double radius;
	};

	class Triangle : public Base
	{
	public:
		Triangle(const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2)
			: v0(v0), v1(v1), v2(v2), E1(v1 - v0), E2(v2 - v0), normal(glm::normalize(glm::cross(E1, E2))) { }

		~Triangle() {};

		virtual bool intersect(const Ray& ray, Intersection& intersection) const;

		glm::dvec3 operator()(double u, double v) const
		{
			return (1 - u - v) * v0 + u * v1 + v * v2;
		}

	private:
		glm::dvec3 v0, v1, v2;

		// Pre-computed edges and normal
		glm::dvec3 E1, E2, normal;
	};
}
