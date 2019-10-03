#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include <iostream>

class Material
{
public:
	Material()
		: reflectance(0.8), emittance(0.0), type(0) { }

	Material(const glm::dvec3& reflectance, const glm::dvec3& emission, uint8_t type = 0)
		: reflectance(reflectance), emittance(emission), type(type) { }

	Material(const glm::dvec3& reflectance, const glm::dvec3& emission, double roughness, bool specular)
		: reflectance(reflectance), emittance(emission), roughness(roughness), specular(specular) 
	{ 
		if (specular)
			type = SPECULAR;
		else if (roughness > 0.001)
			type = OREN_NAYAR;
	}

	enum Type 
	{
		LAMBERTIAN,
		OREN_NAYAR,
		SPECULAR
	};

	glm::dvec3 reflectance, emittance;
	double roughness = 0.0;
	bool specular = 0;
	uint8_t type = LAMBERTIAN;
};