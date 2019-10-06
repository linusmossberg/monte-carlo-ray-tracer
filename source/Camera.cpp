#include "Camera.h"
#include "Random.h"

glm::dvec3 Camera::sampleNaiveRay(const Ray& ray, Scene& scene, size_t ray_depth)
{
	if (ray_depth == max_ray_depth)
	{
		return glm::dvec3(0.0);
	}

	Intersection intersect = scene.intersect(ray);

	if (!intersect)
	{
		return scene.skyColor(ray);
	}

	double terminate = 0.0;
	if (ray_depth > min_ray_depth)
	{
		terminate = 1.0 - intersect.material->reflect_probability;
		if (terminate > rnd(0, 1))
		{
			return glm::dvec3(0.0);
		}
	}

	Ray new_ray;
	new_ray.start = intersect.position + intersect.normal * 0.0000001;
	glm::dvec3 BRDF;

	glm::dvec3 emittance(0.0);
	if (glm::dot(ray.direction, intersect.normal) > 0.0000001)
	{
		emittance = intersect.material->emittance;
	}

	if (intersect.material->type == Material::LAMBERTIAN)
	{
		new_ray.direction = MaterialUtil::CosWeightedSample(intersect.normal);
		BRDF = MaterialUtil::Lambertian(intersect.material.get());

		glm::dvec3 incoming = sampleNaiveRay(new_ray, scene, ray_depth + 1);

		// Cosine term eliminated by cosine-weighted probability density function p(x) = cos(theta)/pi
		// L0 = reflectance * incoming, multiply by pi to eliminate 1/pi from the BRDF.
		return (emittance + (BRDF * incoming * M_PI)) / (1.0 - terminate);
	}
	else //if (intersect.material->type == Material::SPECULAR)
	{
		new_ray.direction = glm::reflect(-ray.direction, intersect.normal);
		BRDF = MaterialUtil::Specular(intersect.material.get());

		glm::dvec3 incoming = sampleNaiveRay(new_ray, scene, ray_depth + 1);

		return (emittance + (BRDF * incoming)) / (1.0 - terminate);
	}
}

glm::dvec3 Camera::sampleExplicitLightRay(Ray ray, Scene& scene, size_t ray_depth)
{
	if (ray_depth == max_ray_depth)
	{
		return glm::dvec3(0.0);
	}

	Intersection intersect = scene.intersect(ray, true);

	if (!intersect)
	{
		return scene.skyColor(ray);
	}

	double terminate = 0.0;
	if (ray_depth > min_ray_depth)
	{
		terminate = 1.0 - intersect.material->reflect_probability;
		if (terminate > rnd(0, 1))
		{
			return glm::dvec3(0.0);
		}
	}

	glm::dvec3 BRDF;

	Ray new_ray;
	new_ray.start = intersect.position + intersect.normal * 0.0000001;
	new_ray.medium_ior = ray.medium_ior;

	if (intersect.surface->material->type == Material::LAMBERTIAN)
	{
		new_ray.direction = MaterialUtil::CosWeightedSample(intersect.normal);

		BRDF = MaterialUtil::Lambertian(intersect.material.get());

		glm::dvec3 emittance = double(ray_depth == 0 || ray.specular) * intersect.material->emittance;
		glm::dvec3 direct = scene.sampleLights(intersect) * BRDF;
		glm::dvec3 indirect = sampleExplicitLightRay(new_ray, scene, ray_depth + 1) * BRDF * M_PI;

		return (emittance + direct + indirect) / (1.0 - terminate);
	}
	else if (intersect.surface->material->type == Material::OREN_NAYAR)
	{
		CoordinateSystem cs(intersect.normal);

		new_ray.direction = MaterialUtil::CosWeightedSample(cs);

		BRDF = MaterialUtil::OrenNayar(intersect.material.get(), new_ray.direction, -ray.direction, cs);

		glm::dvec3 emittance = double(ray_depth == 0 || ray.specular) * intersect.material->emittance;
		glm::dvec3 direct = scene.sampleLights(intersect) * BRDF;
		glm::dvec3 indirect = sampleExplicitLightRay(new_ray, scene, ray_depth + 1) * BRDF * M_PI;

		return (emittance + direct + indirect) / (1.0 - terminate);
	}
	else if (intersect.surface->material->type == Material::TRANSPARENT)
	{
		double n1 = ray.medium_ior; 
		double n2 = abs(ray.medium_ior - scene.ior) < 1e-7 ? intersect.material->ior : scene.ior;

		double R = MaterialUtil::Fresnel(n1, n2, intersect.normal, -ray.direction);

		new_ray.direction = glm::refract(ray.direction, intersect.normal, n1 / n2);
		// glm has a bug and returns NaN on all components for brewster angle (sqrt(k) w/o checking k >= 0 properly)
		if (R > rnd(0.0, 1.0) || std::isnan(new_ray.direction.x))
		{
			new_ray.direction = glm::reflect(ray.direction, intersect.normal);
			new_ray.medium_ior = n1;
		}
		else
		{
			new_ray.start = intersect.position - intersect.normal * 0.0000001;
			new_ray.medium_ior = n2;
		}
		new_ray.specular = true;

		BRDF = MaterialUtil::Specular(intersect.material.get()); // R or (1-R) is eliminated
		glm::dvec3 indirect = sampleExplicitLightRay(new_ray, scene, ray_depth + 1) * BRDF;

		return indirect / (1.0 - terminate);
	}
	else
	{
		double n1 = ray.medium_ior;
		double n2 = intersect.material->ior;

		double R = MaterialUtil::Fresnel(n1, n2, intersect.normal, -ray.direction);

		if (R > rnd(0.0, 1.0))
		{
			new_ray.direction = glm::reflect(ray.direction, intersect.normal);
			new_ray.specular = true;

			BRDF = MaterialUtil::Specular(intersect.material.get()); // R is eliminated
			glm::dvec3 indirect = sampleExplicitLightRay(new_ray, scene, ray_depth + 1) * BRDF;

			return indirect / (1.0 - terminate);
		}
		else
		{
			new_ray.direction = MaterialUtil::CosWeightedSample(intersect.normal);

			BRDF = MaterialUtil::Lambertian(intersect.material.get()); // (1-R) is eliminated

			glm::dvec3 emittance = double(ray_depth == 0 || ray.specular) * intersect.material->emittance;
			glm::dvec3 direct = scene.sampleLights(intersect) * BRDF;
			glm::dvec3 indirect = sampleExplicitLightRay(new_ray, scene, ray_depth + 1) * BRDF * M_PI;

			return (emittance + direct + indirect) / (1.0 - terminate);
		}
	}
}

void Camera::samplePixel(size_t x, size_t y, size_t supersamples, Scene& scene)
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

			image(x, y) += sampleExplicitLightRay(Ray(eye, sensor_pos), scene, 0);
		}
	}
	image(x, y) /= pow2((double)supersamples);
}

void Camera::sampleImage(size_t supersamples, Scene& scene)
{
	std::function<void(Camera*, size_t, Scene&, size_t, size_t)> f = &Camera::sampleImageThread;

	std::vector<std::unique_ptr<std::thread>> threads(std::thread::hardware_concurrency() - 1);
	for (size_t thread = 0; thread < threads.size(); thread++)
	{
		threads[thread] = std::make_unique<std::thread>(f, this, supersamples, std::ref(scene), thread, threads.size());
	}

	for (auto &thread : threads)
	{
		thread->join();
	}
}

std::pair<size_t, size_t> Camera::max_t = std::make_pair(0, 0);

void Camera::sampleImageThread(size_t supersamples, Scene& scene, size_t thread, size_t num_threads)
{
	Random::seed(std::random_device{}()); // Each thread uses different seed.

	std::deque<double> times;
	const size_t N = 10;

	size_t step = (size_t)floor((double)image.width / num_threads);
	size_t start = thread * step;
	size_t end = thread != (num_threads - 1) ? start + step : image.width;
	
	auto t_before = std::chrono::high_resolution_clock::now();
	for (size_t x = start; x < end; x++)
	{
		for (size_t y = 0; y < image.height; y++)
		{
			samplePixel(x, y, supersamples, scene);
		}

		auto t_after = std::chrono::high_resolution_clock::now();
		auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(t_after - t_before);
		t_before = std::chrono::high_resolution_clock::now();

		times.push_back((double)delta_t.count());
		if (times.size() > N)
			times.pop_front();

		double moving_average = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
		size_t msec_left = size_t((end - (x + 1)) * moving_average);

		if ((thread != max_t.first && msec_left > max_t.second) || thread == max_t.first)
		{
			max_t = std::make_pair(thread, msec_left);
			writeTimeDuration(msec_left, thread, std::cout);
		}
	}
}
