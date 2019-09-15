#include "Camera.h"

glm::dvec3 Camera::sampleRay(const Ray& ray, Scene& scene, int ray_depth, Surface::Base* ignore)
{
	Intersection intersect;
	Surface::Base* next_ignore = nullptr;
	for (const auto& surface : scene.surfaces)
	{
		if (surface.get() != ignore)
		{
			Intersection t_intersect;
			if (surface->intersect(ray, t_intersect))
			{
				if (t_intersect.t < intersect.t)
				{
					intersect = t_intersect;
					next_ignore = surface.get();
				}
			}
		}
	}

	if (intersect && ray_depth > 0)
	{
		ray_depth--;
		Ray reflect(intersect.position, intersect.position + glm::reflect(ray.direction, intersect.normal));
		return sampleRay(reflect, scene, ray_depth, next_ignore);
	}

	return double(bool(intersect)) * glm::dvec3(pow(glm::dot(intersect.normal, -ray.direction) + 0.2, 0.5) * pow(1 - intersect.t / (5.15), 0.5));
}

glm::dvec3 Camera::sampleDiffuseRay(const Ray& ray, Scene& scene, int ray_depth, Surface::Base* ignore)
{
	if (ray_depth <= 0)
	{
		return glm::dvec3(0.0);
	}

	Intersection intersect;
	Surface::Base* next_ignore = nullptr;
	for (const auto& surface : scene.surfaces)
	{
		if (surface.get() != ignore)
		{
			Intersection t_intersect;
			if (surface->intersect(ray, t_intersect))
			{
				if (t_intersect.t < intersect.t)
				{
					intersect = t_intersect;
					next_ignore = surface.get();
				}
			}
		}
	}

	double emittance = 0.0;
	Intersection t_intersect;
	if (scene.light.get() != next_ignore && scene.light->intersect(ray, t_intersect))
	{
		if (t_intersect.t < intersect.t)
		{
			emittance = 2.0;
			intersect = t_intersect;
			next_ignore = scene.light.get();
		}
	}

	if (!intersect)
	{
		return glm::dvec3(0.0);
	}

	Ray reflect(intersect.position, intersect.position + glm::dvec3(1.0));

	double inclination = rnd(0.0, M_PI/2.0);
	double azimuth = rnd(0.0, 2.0*M_PI);

	glm::dvec3 tangent = glm::normalize(glm::cross(intersect.normal, intersect.normal + glm::dvec3(1.0)));
	reflect.direction = glm::rotate(glm::rotate(intersect.normal, inclination, tangent), azimuth, intersect.normal);

	double p = 1.0 / (2.0 * M_PI);
	double cos_theta = glm::dot(reflect.direction, intersect.normal);
	double BRDF = 0.9 / M_PI;

	glm::dvec3 incoming = sampleDiffuseRay(reflect, scene, ray_depth - 1, next_ignore);

	return emittance + (BRDF * incoming * cos_theta / p);
}

void Camera::samplePixel(int x, int y, int supersamples, Scene& scene)
{
	double pixel_size = sensor_width / image.width;
	double sub_step = 1.0 / supersamples;

	for (int s_x = 0; s_x < supersamples; s_x++)
	{
		for (int s_y = 0; s_y < supersamples; s_y++)
		{
			glm::dvec2 pixel_space_pos(x + s_x * sub_step + rnd(0.0, sub_step), y + s_y * sub_step + rnd(0.0, sub_step));
			glm::dvec2 center_offset = pixel_size * (glm::dvec2(image.width, image.height) / 2.0 - pixel_space_pos);
			glm::dvec3 sensor_pos = eye + forward * focal_length + left * center_offset.x + up * center_offset.y;

			image(x, y) += sampleDiffuseRay(Ray(eye, sensor_pos), scene, 8);
		}
	}
	image(x, y) /= pow(supersamples, 2);
}

void Camera::sampleImage(int supersamples, Scene& scene)
{
	unsigned sec_before = (unsigned)std::time(nullptr);
	for (int x = 0; x < image.width; x++)
	{
		for (int y = 0; y < image.height; y++)
		{
			samplePixel(x, y, supersamples, scene);
		}

		if (x % (image.width / 64) == 0)
		{
			unsigned sec_after = (unsigned)std::time(nullptr);

			unsigned sec_left = unsigned(((double(image.width) / x) - 1.0) * (sec_after - sec_before));

			unsigned hours = (unsigned)floor(sec_left / (60.0 * 60.0));
			unsigned minutes = (unsigned)floor((sec_left - hours * 60 * 60) / 60.0);
			unsigned seconds = sec_left - hours * 60 * 60 - minutes * 60;

			std::cout << hours << " hours, " << minutes << " minutes and " << seconds << " seconds left." << std::endl;
		}
	}
}
