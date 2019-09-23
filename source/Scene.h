#pragma once

#include <vector>
#include <memory>

#include "Surface.h"
#include "Ray.h"

class Scene
{
public:
	Intersection intersect(const Ray& ray)
	{
		Intersection intersect;
		for (const auto& surface : surfaces)
		{
			Intersection t_intersect;
			if (surface->intersect(ray, t_intersect))
			{
				if (t_intersect.t < intersect.t)
				{
					intersect = t_intersect;
					intersect.surface = surface;
				}
			}
		}
		return intersect;
	}

	glm::dvec3 estimateDirect(const Intersection &intersection, Surface::Base* light)
	{
		if (intersection.material->type != Material::SPECULAR)
		{
			glm::dvec3 light_pos = (*light)(rnd(0, 1), rnd(0, 1));
			Ray shadow_ray(intersection.position + intersection.normal * 0.0000001, light_pos);

			double cos_theta = glm::dot(shadow_ray.direction, intersection.normal);

			Intersection shadow_intersection = intersect(shadow_ray);

			if (shadow_intersection.surface.get() == light && glm::length(shadow_intersection.position - light_pos) < 0.0000001 && cos_theta > 0.0)
			{
				double cos_light_theta = glm::clamp(glm::dot(shadow_intersection.normal, -shadow_ray.direction), 0.0, 1.0);
				double t = light->area() * cos_light_theta / pow(shadow_intersection.t, 2.0);
				return light->material->emittance * t * cos_theta;
			}
		}
		return glm::dvec3(0.0);
	}

	glm::dvec3 sampleLights(const Intersection &intersection)
	{
		glm::dvec3 direct_light(0.0);
		for (const auto &light : emissives)
		{
			if (light == intersection.surface)
			{
				continue;
			}
			direct_light += estimateDirect(intersection, light.get());
		}
		return direct_light;
	}

	void findEmissive()
	{
		for (const auto &surface : surfaces)
		{
			if (glm::length(surface->material->emittance) >= 0.0000001)
			{
				emissives.push_back(surface);
			}
		}
	}

	glm::dvec3 skyColor(const Ray& ray) const
	{
		double f = (glm::dot(glm::dvec3(0.0, 1.0, 0.0), ray.direction) * 0.7 + 1.0) / 2.0;
		if (f < 0.5)
		{
			return glm::dvec3(0.5); // ground color
		}
		return glm::dvec3(1.0 - f, 0.5, f);
	}

	std::vector<std::shared_ptr<Surface::Base>> surfaces;
	std::vector<std::shared_ptr<Surface::Base>> emissives; // subset of surfaces
};