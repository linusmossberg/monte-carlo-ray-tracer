#pragma once

#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include "Scene.h"
#include "Surface.h"
#include "Image.h"
#include "Util.h"

class Camera
{
public:
	Camera(glm::dvec3 eye, glm::dvec3 forward, glm::dvec3 up, double focal_length, double sensor_width, int width, int height) 
		: eye(eye), 
		forward(forward), 
		left(glm::cross(up, forward)), 
		up(up), 
		focal_length(mm2m(focal_length)), 
		sensor_width(mm2m(sensor_width)), 
		image(width, height) { }

	glm::dvec3 sampleRay(const Ray& ray, Scene& scene, int ray_depth, Surface::Base* ignore = nullptr);

	void samplePixel(int x, int y, int supersamples, Scene& scene);

	void sampleImage(int supersamples, Scene& scene);

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
