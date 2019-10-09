#include "Camera.h"
#include "Random.h"

glm::dvec3 Camera::sampleRay(Ray ray, size_t ray_depth)
{
	if (ray_depth == max_ray_depth)
	{
		return glm::dvec3(0.0);
	}

	Intersection intersect = scene->intersect(ray, true);

	if (!intersect)
	{
		return scene->skyColor(ray);
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

	Ray new_ray(intersect.position, 1e-7);

	glm::dvec3 emittance = (ray_depth == 0 || ray.specular || naive) ? intersect.material->emittance : glm::dvec3(0);
	glm::dvec3 direct(0), indirect(0);
	glm::dvec3 BRDF;

	double n1 = ray.medium_ior;
	double n2 = abs(ray.medium_ior - scene->ior) < 1e-7 ? intersect.material->ior : scene->ior;

	if (intersect.material->perfect_specular)
	{
		/* SPECULAR REFLECT */
		BRDF = intersect.material->Specular();
		new_ray.specularReflect(ray.direction, intersect.normal, n1);
	}
	else
	{
		double R = MaterialUtil::Fresnel(n1, n2, intersect.normal, -ray.direction); // Fresnel factor

		if (R > rnd(0, 1))
		{
			/* SPECULAR REFLECT */
			BRDF = intersect.material->Specular();
			new_ray.specularReflect(ray.direction, intersect.normal, n1);
		}
		else
		{
			if (intersect.material->transparency > rnd(0, 1))
			{
				/* TRANSMISSION */
				BRDF = intersect.material->Specular();
				new_ray.specularRefract(ray.direction, intersect.normal, n1, n2);
			}
			else
			{
				/* DIFFUSE REFLECTION */
				CoordinateSystem cs(intersect.normal);

				new_ray.diffuseReflect(cs, n1);
				BRDF = intersect.material->Diffuse(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction));

				direct = naive ? glm::dvec3(0) : scene->sampleLights(intersect) * BRDF;
				BRDF *= M_PI;
			}
		}
	}

	indirect = sampleRay(new_ray, ray_depth + 1) * BRDF;

	return (emittance + direct + indirect) / (1.0 - terminate);
}

void Camera::samplePixel(size_t x, size_t y)
{
	double pixel_size = sensor_width / image.width;
	double sub_step = 1.0 / scene->sqrtspp;

	for (int s_x = 0; s_x < scene->sqrtspp; s_x++)
	{
		for (int s_y = 0; s_y < scene->sqrtspp; s_y++)
		{
			glm::dvec2 pixel_space_pos(x + s_x * sub_step + rnd(0.0, sub_step), y + s_y * sub_step + rnd(0.0, sub_step));
			glm::dvec2 center_offset = pixel_size * (glm::dvec2(image.width, image.height) / 2.0 - pixel_space_pos);

			glm::dvec3 sensor_pos = eye + forward * focal_length + left * center_offset.x + up * center_offset.y;

			image(x, y) += sampleRay(Ray(eye, sensor_pos));
		}
	}
	image(x, y) /= pow2(static_cast<double>(scene->sqrtspp));
}

void Camera::sampleImage(Scene& s)
{
	scene = std::make_shared<Scene>(s);

	std::function<void(Camera*, size_t, size_t)> f = &Camera::sampleImageThread;

	std::vector<std::unique_ptr<std::thread>> threads(std::thread::hardware_concurrency() - 1);
	for (size_t thread = 0; thread < threads.size(); thread++)
	{
		threads[thread] = std::make_unique<std::thread>(f, this, thread, threads.size());
	}

	for (auto &thread : threads)
	{
		thread->join();
	}
}

std::pair<size_t, size_t> Camera::max_t = std::make_pair(0, 0);

void Camera::sampleImageThread(size_t thread, size_t num_threads)
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
			samplePixel(x, y);
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
