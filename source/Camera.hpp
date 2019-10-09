#pragma once

#include <iostream>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <deque>
#include <numeric>
#include <functional>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "Scene.hpp"
#include "Surface.hpp"
#include "Image.hpp"
#include "Util.hpp"

class Camera
{
public:
	Camera(glm::dvec3 eye, glm::dvec3 forward, glm::dvec3 up, double focal_length, double sensor_width, size_t width, size_t height) 
		: eye(eye),
		forward(glm::normalize(forward)),
		left(glm::cross(glm::normalize(up), glm::normalize(forward))),
		up(glm::normalize(up)),
		focal_length(focal_length/1000.0),
		sensor_width(sensor_width/1000.0),
		image(width, height) { }

	Camera(glm::dvec3 eye, glm::dvec3 look_at, double focal_length, double sensor_width, size_t width, size_t height)
		: eye(eye),
		focal_length(focal_length/1000.0),
		sensor_width(sensor_width/1000.0),
		image(width, height)
	{
		lookAt(look_at);
	}

	void sampleImage(Scene& s);

	void saveImage(const std::string& filename) const
	{
		image.save(filename);
	}

	void setPosition(const glm::dvec3& p)
	{
		eye = p;
	}

	void setNaive(bool n)
	{
		naive = n;
	}

	void lookAt(const glm::dvec3& p)
	{
		forward = glm::normalize(p - eye);
		left = glm::normalize(glm::cross(glm::dvec3(0.0, 1.0, 0.0), forward));
		up = glm::normalize(glm::cross(forward, left));
	}

private:

	glm::dvec3 sampleRay(Ray ray, size_t ray_depth = 0);
	void samplePixel(size_t x, size_t y);
	void sampleImageThread(size_t thread, size_t num_threads);

	void printInfoThread();

	glm::dvec3 eye;
	glm::dvec3 forward, left, up;

	double focal_length, sensor_width;
	Image image;

	size_t min_ray_depth = 3;
	size_t max_ray_depth = 512; // prevent call stack overflow

	std::shared_ptr<Scene> scene;

	std::atomic_size_t num_sampled_pixels = 0;
	size_t last_num_sampled_pixels = 0;
	std::chrono::time_point<std::chrono::steady_clock> last_update = std::chrono::steady_clock::now();
	const size_t num_times = 32;
	std::deque<double> times;

	bool naive = false;
};
