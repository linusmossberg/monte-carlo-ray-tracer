#pragma once

#include <iostream>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <deque>
#include <numeric>
#include <functional>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "Scene.h"
#include "Surface.h"
#include "Image.h"
#include "Util.h"

class Camera
{
public:
	Camera(glm::dvec3 eye, glm::dvec3 forward, glm::dvec3 up, double focal_length, double sensor_width, size_t width, size_t height) 
		: eye(eye), 
		forward(forward), 
		left(glm::cross(up, forward)), 
		up(up), 
		focal_length(mm2m(focal_length)), 
		sensor_width(mm2m(sensor_width)), 
		image(width, height) { }

	glm::dvec3 sampleNaiveRay(const Ray& ray, Scene& scene, size_t ray_depth);
	glm::dvec3 sampleExplicitLightRay(Ray ray, Scene& scene, size_t ray_depth);

	void samplePixel(size_t x, size_t y, size_t supersamples, Scene& scene);
	void sampleImage(size_t supersamples, Scene& scene);
	void saveImage(const std::string& filename) const
	{
		image.save(filename);
	}

	void setPosition(const glm::dvec3& p)
	{
		eye = p;
	}

	void lookAt(const glm::dvec3& p)
	{
		forward = glm::normalize(p - eye);
		left = glm::cross(glm::dvec3(0.0, 1.0, 0.0), forward);
		up = glm::cross(forward, left);
	}

private:
	void sampleImageThread(size_t supersamples, Scene& scene, size_t thread, size_t num_threads);

	glm::dvec3 eye;
	glm::dvec3 forward, left, up;

	double focal_length, sensor_width;
	Image image;

	size_t min_ray_depth = 3;
	size_t max_ray_depth = 512; // prevent call stack overflow

	// Keeps track of the thread with most estimated time remaining
	static std::pair<size_t, size_t> max_t; // <thread, msec_left>
};
