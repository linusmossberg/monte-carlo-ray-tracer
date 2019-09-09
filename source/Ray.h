#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

class Ray
{
public:
	Ray(const glm::vec3& start, const glm::vec3& end)
		: start(start), direction(glm::normalize(end - start)) { }

	glm::vec3 operator()(float t) const
	{
		return start + direction * t;
	}
	
	// Normalized direction -> t corresponds to euclidian distance in metric units
	glm::vec3 start, direction;
};

struct Intersection
{
	glm::vec3 position;
	glm::vec3 normal;
	float t = std::numeric_limits<float>::max();

	explicit operator bool() const
	{
		return t != std::numeric_limits<float>::max();
	}
};