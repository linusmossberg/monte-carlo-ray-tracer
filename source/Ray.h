#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Surface.h"
#include "Material.h"

namespace Surface { class Base; }

class Ray
{
public:
	Ray()
		: start(0), direction(0, 0, 1), offset(1e-7) { }

	Ray(const glm::dvec3& start, const glm::dvec3& end, double offset = 1e-7)
		: start(start), direction(glm::normalize(end - start)) { }

	Ray(const glm::dvec3& start, double offset = 1e-7)
		: start(start), direction(0,0,1), offset(offset) { }

	glm::dvec3 operator()(double t) const
	{
		return start + direction * t;
	}

	void diffuseReflect(const CoordinateSystem& cs, double n1)
	{
		direction = cs.localToGlobal(MaterialUtil::CosWeightedSample());
		start += cs.normal * offset;
		specular = false;
		medium_ior = n1;
	}

	void specularReflect(const glm::dvec3 &in, const glm::dvec3 &normal, double n1)
	{
		direction = glm::reflect(in, normal);
		start += normal * offset;
		specular = true;
		medium_ior = n1;
	}

	void specularRefract(const glm::dvec3 &in, const glm::dvec3 &normal, double n1, double n2)
	{
		direction = glm::refract(in, normal, n1 / n2);
		if (std::isnan(direction.x))
		{
			/* SPECULAR REFLECT, BREWSTER ANGLE */
			specularReflect(in, normal, n1);
		}
		else
		{
			/* SPECULAR REFRACT */
			start -= normal * offset;
			specular = true;
			medium_ior = n2;
		}
	}
	
	// Normalized direction -> t corresponds to euclidian distance in metric units
	glm::dvec3 start, direction;
	double medium_ior = 1.0;
	double offset;
	bool specular = false;
};

struct Intersection
{
	glm::dvec3 position;
	glm::dvec3 normal;
	double t = std::numeric_limits<double>::max();
	std::shared_ptr<Material> material;

	explicit operator bool() const
	{
		return t != std::numeric_limits<double>::max();
	}
};