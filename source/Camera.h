#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "Scene.h"
#include "Surface.h"
#include "Image.h"
#include "Util.h"

float t = 0.0f;

class Camera
{
public:
	Camera(glm::vec3 eye, glm::vec3 forward, glm::vec3 up, float focal_length, float sensor_width, int width, int height)
		: eye(eye), forward(forward), left(glm::cross(up, forward)), up(up), focal_length(mm2m(focal_length)), sensor_width(mm2m(sensor_width)), image(width, height) { }

	void samplePixel(int x, int y, int samples, Scene& scene)
	{
		float pixel_size = sensor_width / image.width;

		for (int sample = 0; sample < samples; sample++)
		{ 
			glm::vec2 center_offset = pixel_size * (glm::vec2(image.width, image.height) / 2.0f - glm::vec2(x + rnd(0, 1), y + rnd(0, 1)));
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
				//image(x, y) += (glm::dot(intersect.normal, -camera_ray.direction) + 0.5f) * ((intersect.normal + 1.0f) * 0.5f);
				image(x, y) += pow(glm::dot(intersect.normal, -camera_ray.direction) + 0.2f, 0.5f) * pow(1 - intersect.t / 5.14651f, 0.5f);
			}
		}
		image(x, y) /= samples;
	}

	void sampleImage(int samples, Scene& scene)
	{
		for (int x = 0; x < image.width; x++)
		{
			for (int y = 0; y < image.height; y++)
			{
				samplePixel(x, y, samples, scene);
			}
			if (x % (image.width/10) == 0) { std::cout << (100.0 * x / image.width) << "%" << std::endl; }
		}
	}

	void saveImage(const std::string& filename) const
	{
		image.save(filename);
	}

private:
	glm::vec3 eye;

	glm::vec3 forward, left, up;

	float focal_length, sensor_width;
	Image image;
};
