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
		terminate = 1.0 - glm::length(intersect.material->reflectance) / sqrt(3.0);
		if (terminate > rnd(0, 1))
		{
			return glm::dvec3(0.0);
		}
	}

	Ray reflect;
	reflect.start = intersect.position + intersect.normal * 0.0000001;
	glm::dvec3 BRDF;

	if (intersect.material->type == Material::LAMBERTIAN)
	{
		// Generate uniform sample on unit circle at radius r and angle azimuth
		double r = sqrt(rnd(0.0, 1.0));
		double azimuth = rnd(0.0, 2.0*M_PI);

		// Project up to hemisphere.
		// The result is a cosine-weighted hemispherical sample.
		glm::dvec3 local_dir(r * cos(azimuth), r * sin(azimuth), sin(acos(r)));

		reflect.direction = CoordinateSystem::localToGlobalUnitVector(local_dir, intersect.normal);
		BRDF = intersect.material->reflectance / M_PI;

		glm::dvec3 incoming = sampleNaiveRay(reflect, scene, ray_depth + 1);

		// Cosine term eliminated by cosine-weighted probability density function p(x) = cos(theta)/pi
		// L0 = reflectance * incoming, multiply by pi to eliminate 1/pi from the BRDF.
		return (intersect.material->emittance + (BRDF * incoming * M_PI)) / (1.0 - terminate);
	}
	else //if (intersect.material->type == Material::SPECULAR)
	{
		reflect.direction = glm::reflect(ray.direction, intersect.normal);
		BRDF = intersect.material->reflectance;

		glm::dvec3 incoming = sampleNaiveRay(reflect, scene, ray_depth + 1);

		return (intersect.material->emittance + (BRDF * incoming)) / (1.0 - terminate);
	}
}

glm::dvec3 Camera::sampleExplicitLightRay(Ray ray, Scene& scene, size_t ray_depth)
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
		terminate = 1.0 - glm::length(intersect.material->reflectance) / sqrt(3.0);
		if (terminate > rnd(0, 1))
		{
			return glm::dvec3(0.0);
		}
	}

	glm::dvec3 BRDF;

	Ray reflect;
	reflect.start = intersect.position + intersect.normal * 0.0000001;

	if (intersect.surface->material->type == Material::LAMBERTIAN)
	{
		// Generate random cosine-weighted sample on hemisphere for our new direction
		double r = sqrt(rnd(0.0, 1.0));
		double azimuth = rnd(0.0, 2.0*M_PI);
		glm::dvec3 local_dir(r * cos(azimuth), r * sin(azimuth), sin(acos(r)));

		reflect.direction = CoordinateSystem::localToGlobalUnitVector(local_dir, intersect.normal);

		BRDF = intersect.material->reflectance / M_PI;

		glm::dvec3 emittance = double(ray_depth == 0 || (ray.specular && ray_depth == 1)) * intersect.material->emittance;
		glm::dvec3 direct = scene.sampleLights(intersect) * BRDF;
		glm::dvec3 indirect = sampleExplicitLightRay(reflect, scene, ray_depth + 1) * BRDF * M_PI;

		return (emittance + direct + indirect) / (1.0 - terminate);
	}
	else if (intersect.surface->material->type == Material::ORENNAYAR)
	{
		double r = sqrt(rnd(0.0, 1.0));
		double azimuth = rnd(0.0, 2.0*M_PI);
		glm::dvec3 local_dir(r * cos(azimuth), r * sin(azimuth), sin(acos(r)));

		CoordinateSystem cs(intersect.normal);

		reflect.direction = cs.localToGlobal(local_dir);

		glm::dvec3 o = cs.globalToLocal(-ray.direction);
		glm::dvec3 i = cs.globalToLocal(reflect.direction);
		glm::dvec3 color = intersect.material->reflectance;
		double roughness = 0.3;

		double variance = pow2(roughness);
		double A = 1.0 - 0.5 * variance / (variance + 0.33);
		double B = 0.45 * variance / (variance + 0.09);

		// equivalent to dot(normalize(i.x, i.y, 0), normalize(o.x, o.y, 0)), 
		// i.e. remove z-component (normal) and get the cos angle between vectors with dot
		double cos_delta_phi = glm::clamp((i.x*o.x + i.y*o.y) / sqrt((pow2(i.x) + pow2(i.y)) * (pow2(o.x) + pow2(o.y))), 0.0, 1.0);

		// C = sin(alpha) * tan(beta), i.z = dot(i, (0,0,1))
		double C = sqrt((1.0 - pow2(i.z)) * (1.0 - pow2(o.z))) / std::max(i.z, o.z);

		BRDF = (color / M_PI) * (A + B * cos_delta_phi * C);

		glm::dvec3 emittance = double(ray_depth == 0 || (ray.specular && ray_depth == 1)) * intersect.material->emittance;
		glm::dvec3 direct = scene.sampleLights(intersect) * BRDF;
		glm::dvec3 indirect = sampleExplicitLightRay(reflect, scene, ray_depth + 1) * BRDF * M_PI;

		return (emittance + direct + indirect) / (1.0 - terminate);
	}
	else //if (intersect.surface->material->type == Material::SPECULAR)
	{
		reflect.direction = glm::reflect(ray.direction, intersect.normal);
		reflect.specular = true;

		BRDF = intersect.material->reflectance;

		glm::dvec3 indirect = sampleExplicitLightRay(reflect, scene, ray_depth + 1) * BRDF;

		return indirect / (1.0 - terminate);
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
