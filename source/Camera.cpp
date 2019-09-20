#include "Camera.h"
#include "Random.h"

glm::dvec3 Camera::sampleRay(const Ray& ray, Scene& scene, int ray_depth, Surface::Base* ignore)
{
	if (ray_depth <= 0)
	{
		return glm::dvec3(0.0);
	}

	Intersection intersect = scene.intersect(ray, ignore);

	if (!intersect)
	{
		return glm::dvec3(0.0);
	}

	Ray reflect;
	reflect.start = intersect.position;
	glm::dvec3 BRDF;

	if (intersect.material->type == Material::LAMBERTIAN)
	{
		// Generate uniform sample on unit circle at radius r and angle azimuth
		double r = sqrt(rnd(0.0, 1.0));
		double azimuth = rnd(0.0, 2.0*M_PI);

		// Project up to hemisphere by rotating up angle inclination. 
		// The result is a cosine-weighted hemispherical sample.
		double inclination = acos(r);

		glm::dvec3 tangent = orthogonalUnitVector(intersect.normal);
		reflect.direction = glm::rotate(glm::rotate(intersect.normal, inclination, tangent), azimuth, intersect.normal);

		BRDF = intersect.material->reflectance / M_PI;
	}
	else //if (intersect.material->type == Material::SPECULAR)
	{
		reflect.direction = glm::reflect(ray.direction, intersect.normal);
		BRDF = intersect.material->reflectance;
	}

	glm::dvec3 incoming = sampleRay(reflect, scene, ray_depth - 1, ignore);

	if (intersect.material->type == Material::LAMBERTIAN)
	{
		// Cosine term eliminated by cosine-weighted probability density function p(x) = cos(theta)/pi
		// L0 = reflectance * incoming, multiply by pi to eliminate 1/pi from the BRDF.
		return intersect.material->emittance + (BRDF * incoming * M_PI);
	}
	else //if (intersect.material->type == Material::SPECULAR)
	{
		return intersect.material->emittance + (BRDF * incoming);
	}
}

void Camera::samplePixel(size_t x, size_t y, int supersamples, Scene& scene)
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

			image(x, y) += sampleRay(Ray(eye, sensor_pos), scene, 8);
		}
	}
	image(x, y) /= pow(supersamples, 2);
}

void Camera::sampleImage(int supersamples, Scene& scene)
{
	void (Camera::*f)(int, Scene&, size_t, size_t) = &Camera::sampleImage;

	std::vector<std::unique_ptr<std::thread>> threads(std::thread::hardware_concurrency() - 2);

	size_t step = (size_t)floor((double)image.width / threads.size());
	for (size_t i = 0; i < threads.size(); i++)
	{
		size_t start = i * step;
		size_t end = i != (threads.size() - 1) ? start + step : image.width;
		threads[i] = std::make_unique<std::thread>(f, this, supersamples, std::ref(scene), start, end);
	}

	for (auto &thread : threads)
	{
		thread->join();
	}
}

void Camera::sampleImage(int supersamples, Scene& scene, size_t start, size_t end)
{
	Random::seed(std::random_device{}()); // Each thread uses different seed.

	auto t_before = std::chrono::high_resolution_clock::now();
	size_t x_before = start;
	for (size_t x = start; x < end; x++)
	{
		for (size_t y = 0; y < image.height; y++)
		{
			samplePixel(x, y, supersamples, scene);
		}

		if (end == image.width)
		{
			auto t_after = std::chrono::high_resolution_clock::now();
			auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(t_after - t_before);
			size_t delta_x = x - x_before;
			if (delta_t.count() >= 10000 && delta_x > 0)
			{
				size_t msec_left = size_t((end - x) * 1.0 * delta_t.count() / delta_x);

				writeTimeDuration(msec_left, std::cout);

				t_before = std::chrono::high_resolution_clock::now();
				x_before = x;
			}
		}
	}
}
