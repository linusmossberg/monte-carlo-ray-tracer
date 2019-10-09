#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Util.h"

class Material
{
public:
	Material() : reflectance(0.8), specular_reflectance(0.0), emittance(0.0) { }

	Material(const glm::dvec3& reflectance, const glm::dvec3& specular_reflectance, const glm::dvec3& emission, double roughness, double ior, double transparency, bool perfect_specular) 
		: reflectance(reflectance),
		specular_reflectance(specular_reflectance),
		emittance(emission),
		roughness(roughness), 
		ior(ior),
		transparency(transparency),
		perfect_specular(perfect_specular)
	{
		reflect_probability = calculateReflectProbability();
	}

	glm::dvec3 Diffuse(const glm::dvec3 &i, const glm::dvec3 &o);
	glm::dvec3 Specular();
	glm::dvec3 Lambertian();
	glm::dvec3 OrenNayar(const glm::dvec3 &i, const glm::dvec3 &o);

	glm::dvec3 reflectance, specular_reflectance, emittance;
	double roughness, ior, transparency, reflect_probability;

	// Represents ior = infinity -> fresnel factor = 1.0 -> all rays specularly reflected
	bool perfect_specular; 

private:
	// Used for russian roulette path termination
	double calculateReflectProbability();
};

namespace MaterialUtil
{
	inline glm::dvec3 CosWeightedSample()
	{
		// Generate uniform sample on unit circle at radius r and angle azimuth
		double r = sqrt(rnd(0.0, 1.0));
		double azimuth = rnd(0.0, 2.0*M_PI);

		// Project up to hemisphere.
		// The result is a cosine-weighted hemispherical sample.
		glm::dvec3 local_dir(r * cos(azimuth), r * sin(azimuth), sin(acos(r)));

		return local_dir;
	}

	// Schlick's approximation of fresnel factor
	inline double Fresnel(double n1, double n2, const glm::dvec3& normal, const glm::dvec3& dir)
	{
		if (abs(n1 - n2) < 1e-7)
			return 0;

		double R0 = pow2((n1 - n2) / (n1 + n2));
		return R0 + (1.0 - R0) * pow(1.0 - glm::dot(normal, dir), 5);
	}
}