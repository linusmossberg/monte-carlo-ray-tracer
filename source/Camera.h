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
	Camera(glm::vec3 eye, glm::vec3 forward, glm::vec3 up, float focal_length, float sensor_width, int width, int height) 
		: eye(eye), 
		forward(forward), 
		left(glm::cross(up, forward)), 
		up(up), 
		focal_length(mm2m(focal_length)), 
		sensor_width(mm2m(sensor_width)), 
		image(width, height) { }

	void samplePixel(int x, int y, int supersamples, Scene& scene);

	void sampleImage(int supersamples, Scene& scene);

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
