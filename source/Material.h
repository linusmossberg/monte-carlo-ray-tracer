#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

class Material
{
public:
	Material()
		: reflectance(0.8), emittance(0.0) { }

	Material(const glm::dvec3& reflectance, const glm::dvec3& emission)
		: reflectance(reflectance), emittance(emission) { }

	glm::dvec3 reflectance, emittance;
};