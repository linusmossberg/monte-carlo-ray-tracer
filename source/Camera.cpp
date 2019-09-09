#include "Camera.h"

void Camera::samplePixel(int x, int y, int supersamples, Scene& scene)
{
	float pixel_size = sensor_width / image.width;
	float sub_step = 1.0f / supersamples;

	for (int s_x = 0; s_x < supersamples; s_x++)
	{
		for (int s_y = 0; s_y < supersamples; s_y++)
		{
			glm::vec2 pixel_space_pos(x + s_x * sub_step + rnd(0.0f, sub_step), y + s_y * sub_step + rnd(0.0f, sub_step));
			glm::vec2 center_offset = pixel_size * (glm::vec2(image.width, image.height) / 2.0f - pixel_space_pos);
			glm::vec3 sensor_pos = eye + forward * focal_length + left * center_offset.x + up * center_offset.y;

			Ray camera_ray(eye, sensor_pos);

			Intersection intersect;
			for (const auto& surface : scene.surfaces)
			{
				Intersection t_intersect;
				if (surface->intersect(camera_ray, t_intersect))
				{
					if (t_intersect.t < intersect.t)
					{
						intersect = t_intersect;
					}
				}
			}

			if (intersect)
			{
				image(x, y) += pow(glm::dot(intersect.normal, -camera_ray.direction) + 0.2f, 0.5f) * pow(1 - intersect.t / 5.15f, 0.5f);
			}
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
