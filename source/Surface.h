#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "Ray.h"
#include "Material.h"
#include "Util.h"

namespace Surface
{
	class Base
	{
	public:
		Base() 
			: material(std::make_shared<Material>()) { };

		Base(const Material& material)
			: material(std::make_shared<Material>(material)) { };

		virtual ~Base() {};

		virtual bool intersect(const Ray& ray, Intersection& intersection) const = 0;

		virtual glm::dvec3 operator()(double u, double v) const = 0; // point on surface

		virtual double area() const = 0;

		std::shared_ptr<Material> material;
	};

	class Sphere : public Base
	{
	public:
		Sphere(const glm::dvec3& origin, double radius)
			: origin(origin), radius(radius) { }

		Sphere(const glm::dvec3& origin, double radius, const Material& material)
			: Base(material), origin(origin), radius(radius) { }

		~Sphere() {};

		virtual bool intersect(const Ray& ray, Intersection& intersection) const;

		virtual glm::dvec3 operator()(double u, double v) const
		{
			double z = 1.0 - 2.0 * u;
			double r = sqrt(fmax(0.0, 1.0 - pow2(z)));
			double phi = 2 * M_PI * v;

			return origin + radius * glm::dvec3(r * cos(phi), r * sin(phi), z);
		}

		virtual double area() const
		{
			return 4.0 * M_PI * pow2(radius);
		}

	private:
		glm::dvec3 origin;
		double radius;
	};

	class Triangle : public Base
	{
	public:
		Triangle(const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2)
			: v0(v0), v1(v1), v2(v2), E1(v1 - v0), E2(v2 - v0), normal(glm::normalize(glm::cross(E2, E1))) { }

		Triangle(const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2, const Material& material)
			: Base(material), v0(v0), v1(v1), v2(v2), E1(v1 - v0), E2(v2 - v0), normal(glm::normalize(glm::cross(E2, E1))) { }

		~Triangle() {};

		virtual bool intersect(const Ray& ray, Intersection& intersection) const;

		virtual glm::dvec3 operator()(double u, double v) const
		{
			double su = sqrt(u);
			u = 1 - su;
			v = v * su;

			return (1 - u - v) * v0 + u * v1 + v * v2;
		}

		virtual double area() const
		{
			return glm::length(glm::cross(E1, E2)) / 2.0;
		}

	private:
		glm::dvec3 v0, v1, v2;

		// Pre-computed edges and normal
		glm::dvec3 E1, E2, normal;
	};
}
