#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Util.h"

class Material
{
public:
	Material() : reflectance(0.8), specular_reflectance(0.0), emittance(0.0) { }

	Material(const glm::dvec3& reflectance, const glm::dvec3& specular_reflectance, const glm::dvec3& emission, double roughness, double ior, bool transparent)
		: reflectance(reflectance), specular_reflectance(specular_reflectance), emittance(emission), roughness(roughness), ior(ior), transparent(transparent)
	{ 
		if (transparent)
			type = TRANSPARENT;
		else if (abs(ior - 1.0) > 1e-7)
			type = SPECULAR_DIFFUSE;
		else if (roughness > 0.001)
			type = OREN_NAYAR;

		reflect_probability = calculateReflectProbability();
	}

	enum Type 
	{
		LAMBERTIAN,
		OREN_NAYAR,
		TRANSPARENT,
		SPECULAR_DIFFUSE
	};

	glm::dvec3 reflectance, specular_reflectance, emittance;
	double roughness, ior, reflect_probability;
	bool transparent;

	uint8_t type = LAMBERTIAN;

private:
	// Used for russian roulette path termination
	double calculateReflectProbability()
	{
		const glm::dvec3 &r = reflectance;
		const glm::dvec3 &s = specular_reflectance;

		return (r.x + r.y + r.z + s.x + s.y + s.z) / 6.0;
	}
};

namespace MaterialUtil
{
	inline glm::dvec3 CosWeightedSample(const glm::dvec3& normal)
	{
		// Generate uniform sample on unit circle at radius r and angle azimuth
		double r = sqrt(rnd(0.0, 1.0));
		double azimuth = rnd(0.0, 2.0*M_PI);

		// Project up to hemisphere.
		// The result is a cosine-weighted hemispherical sample.
		glm::dvec3 local_dir(r * cos(azimuth), r * sin(azimuth), sin(acos(r)));

		return CoordinateSystem::localToGlobalUnitVector(local_dir, normal);
	}

	inline glm::dvec3 CosWeightedSample(const CoordinateSystem& cs)
	{
		double r = sqrt(rnd(0.0, 1.0));
		double azimuth = rnd(0.0, 2.0*M_PI);

		glm::dvec3 local_dir(r * cos(azimuth), r * sin(azimuth), sin(acos(r)));

		return cs.localToGlobal(local_dir);
	}

	inline glm::dvec3 Lambertian(const Material* material)
	{
		return material->reflectance / M_PI;
	}

	inline glm::dvec3 Specular(const Material* material)
	{
		return material->specular_reflectance;
	}

	inline glm::dvec3 OrenNayar(const Material* material, const glm::dvec3 &in, const glm::dvec3 &out, const CoordinateSystem &cs)
	{
		glm::dvec3 i = cs.globalToLocal(in);
		glm::dvec3 o = cs.globalToLocal(out);
		glm::dvec3 color = material->reflectance;

		double variance = pow2(material->roughness);
		double A = 1.0 - 0.5 * variance / (variance + 0.33);
		double B = 0.45 * variance / (variance + 0.09);

		// equivalent to dot(normalize(i.x, i.y, 0), normalize(o.x, o.y, 0)), 
		// i.e. remove z-component (normal) and get the cos angle between vectors with dot
		double cos_delta_phi = glm::clamp((i.x*o.x + i.y*o.y) / sqrt((pow2(i.x) + pow2(i.y)) * (pow2(o.x) + pow2(o.y))), 0.0, 1.0);

		// C = sin(alpha) * tan(beta), i.z = dot(i, (0,0,1))
		double C = sqrt((1.0 - pow2(i.z)) * (1.0 - pow2(o.z))) / std::max(i.z, o.z);

		return (color / M_PI) * (A + B * cos_delta_phi * C);
	}

	// Schlick's approximation of fresnel factor
	inline double Fresnel(double n1, double n2, const glm::dvec3& normal, const glm::dvec3& dir)
	{
		double R0 = pow2((n1 - n2) / (n1 + n2));
		return R0 + (1.0 - R0) * std::pow(1.0 - glm::dot(normal, dir), 5);
	}
}