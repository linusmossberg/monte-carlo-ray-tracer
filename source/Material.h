#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

class Material
{
public:
	Material()
		: reflectance(0.8), emittance(0.0), type(0) { }

	Material(const glm::dvec3& reflectance, const glm::dvec3& emission, uint8_t type = 0)
		: reflectance(reflectance), emittance(emission), type(type) { }

	enum Type 
	{
		LAMBERTIAN,
		SPECULAR
	};

	glm::dvec3 reflectance, emittance;
	uint8_t type;
};