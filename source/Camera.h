#pragma once

#include <iostream>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <chrono>

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

	glm::dvec3 sampleRay(const Ray& ray, Scene& scene, int ray_depth, Surface::Base* ignore = nullptr);

	void samplePixel(size_t x, size_t y, int supersamples, Scene& scene);

	void sampleImage(int supersamples, Scene& scene);

	void sampleImage(int supersamples, Scene& scene, size_t start, size_t end);

	void saveImage(const std::string& filename) const
	{
		image.save(filename);
	}

private:
	glm::dvec3 eye;

	glm::dvec3 forward, left, up;

	double focal_length, sensor_width;
	Image image;
};
