#include "Camera.h"
#include "Random.h"

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

	if (!intersect)
	{
		return glm::dvec3(0.0);
	}

	Ray reflect(intersect.position, intersect.position + glm::dvec3(1.0));

	// Generate uniform sample on unit circle at radius r and angle azimuth
	double r = sqrt(rnd(0.0, 1.0));
	double azimuth = rnd(0.0, 2.0*M_PI);

	// Project up to hemisphere by rotating up angle inclination. 
	// The result is a cosine-weighted hemispherical sample.
	double inclination = acos(r);

	glm::dvec3 tangent = glm::normalize(glm::cross(intersect.normal, intersect.normal + glm::dvec3(1.0)));
	reflect.direction = glm::rotate(glm::rotate(intersect.normal, inclination, tangent), azimuth, intersect.normal);

	glm::dvec3 BRDF = intersect.material->reflectance / M_PI;

	glm::dvec3 incoming = sampleDiffuseRay(reflect, scene, ray_depth - 1, next_ignore);

	// Cosine term eliminated by cosine-weighted probability density function p(x) = cos(theta)/pi
	// L0 = reflectance * incoming, multiply by pi to eliminate 1/pi from the BRDF.
	return intersect.material->emittance + (BRDF * incoming * M_PI);
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
	int x_before = start;
	for (int x = start; x < end; x++)
	{
		for (int y = 0; y < image.height; y++)
		{
			samplePixel(x, y, supersamples, scene);
		}

		if (end == image.width)
		{
			auto t_after = std::chrono::high_resolution_clock::now();
			auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(t_after - t_before);
			int delta_x = x - x_before;
			if (delta_t.count() >= 10000 && delta_x > 0)
			{
				int msec_left = int(double(end - x) * (double(delta_t.count()) / delta_x));
				int sec_left = (int)floor(msec_left / 1000.0);

				int hours = (unsigned)floor(sec_left / (60.0 * 60.0));
				int minutes = (unsigned)floor((sec_left - hours * 60 * 60) / 60.0);
				int seconds = sec_left - hours * 60 * 60 - minutes * 60;

				t_before = std::chrono::high_resolution_clock::now();
				x_before = x;
				std::cout << "Time remaining: "
						  << std::setfill('0') << std::setw(2) << hours << ":" 
					      << std::setfill('0') << std::setw(2) << minutes << ":" 
						  << std::setfill('0') << std::setw(2) << seconds << "."
						  << std::setfill('0') << std::setw(3) << (msec_left - sec_left*1000) << std::endl;
			}
		}
	}
}
