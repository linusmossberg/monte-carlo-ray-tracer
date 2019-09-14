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

			image(x, y) += sampleRay(Ray(eye, sensor_pos), scene, 1);
		}
	}
	image(x, y) /= pow(supersamples, 2);
}

void Camera::sampleImage(int supersamples, Scene& scene)
{
	for (int x = 0; x < image.width; x++)
	{
		for (int y = 0; y < image.height; y++)
		{
			samplePixel(x, y, supersamples, scene);
		}
		if (x % (image.width / 10) == 0) { std::cout << (100.0 * x / image.width) << "%" << std::endl; }
	}
}
