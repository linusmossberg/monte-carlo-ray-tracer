#include <iostream>
#include <string>
#include <ctime>
#include <algorithm>

#include <glm/vec3.hpp>

#include "Scene.h"
#include "Camera.h"

int main()
{
	unsigned seed = (unsigned)std::time(nullptr);
	//seed = 1568013294;
	std::cout << "Seed: " << seed << std::endl;
	std::srand(seed);

	std::vector<glm::vec3> sphere_positions;

	Scene scene;
	scene.surfaces.resize(16);
	std::generate(scene.surfaces.begin(), scene.surfaces.end(), [&sphere_positions]() {
		glm::vec3 new_pos = glm::vec3(rnd(-.6f, .6f), rnd(-.35f, .35f), rnd(-1.f, -4.5f));
		return std::make_unique<Surface::Sphere>(glm::vec3(rnd(-.6f, .6f), rnd(-.35f, .35f), rnd(-1.f, -4.5f)), rnd(.05f, .15f));
	});

	float a = 1.0f, b = 0.7f, c = -5.0f, d = 0.0f;
	glm::vec3 v0(-a, -b, c), v1(a, b, c), v2(-a, b, c), v3(a, -b, c);
	glm::vec3 v4(-a, -b, d), v5(a, -b, d), v6(a, b, d), v7(-a, b, d);

	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v0, v1, v2)); // Back
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v0, v3, v1));	
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v0, v4, v5)); // Bottom
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v0, v5, v3));
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v0, v7, v4)); // Left
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v0, v2, v7));
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v1, v7, v2)); // Top
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v1, v6, v7));
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v1, v3, v5)); // Right
	scene.surfaces.push_back(std::make_unique<Surface::Triangle>(v1, v5, v6));

	Camera camera(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), 36.0f, 35.0f,  960, 540);

	camera.sampleImage(4, scene);
	camera.saveImage("image5.tga");
	
	return 0;
}