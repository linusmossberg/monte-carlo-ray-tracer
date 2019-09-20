#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Material.h"

class Ray
{
public:
	Ray()
		: start(0.0), direction(0.0, 0.0, 1.0) { }

	Ray(const glm::dvec3& start, const glm::dvec3& end)
		: start(start), direction(glm::normalize(end - start)) { }

	glm::dvec3 operator()(double t) const
	{
		return start + direction * t;
	}
	
	// Normalized direction -> t corresponds to euclidian distance in metric units
	glm::dvec3 start, direction;
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