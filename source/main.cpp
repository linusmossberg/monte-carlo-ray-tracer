#include <iostream>
#include <string>
#include <algorithm>

#include <glm/vec3.hpp>

#include "Random.h"
#include "Util.h"
#include "Scene.h"
#include "Camera.h"

int main()
{
	unsigned seed = std::random_device{}();
	std::cout << "Seed: " << seed << std::endl;
	Random::init(seed);

	Scene scene;
	scene.surfaces.resize(16);
	std::generate(scene.surfaces.begin(), scene.surfaces.end(), []() {
		return std::make_shared<Surface::Sphere>(glm::dvec3(rnd(-.6, .6), rnd(-.35, .35), rnd(-1., -4.5)), rnd(.05, .15));
	});

	double a = 1.0, b = 0.7, c = -5.0, d = 0.0;
	glm::dvec3 v0(-a, -b, c), v1(a, b, c), v2(-a, b, c), v3(a, -b, c);
	glm::dvec3 v4(-a, -b, d), v5(a, -b, d), v6(a, b, d), v7(-a, b, d);

	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v1, v2)); // Back
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v3, v1));	
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v4, v5)); // Bottom
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v5, v3));
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v7, v4)); // Left
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v2, v7));
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v1, v7, v2)); // Top
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v1, v6, v7));
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v1, v3, v5)); // Right
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v1, v5, v6));
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v7, v6, v5)); // Front
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v7, v5, v4));

	Camera camera(glm::dvec3(0.0, 0.0, -0.01), glm::dvec3(0.0, 0.0, -1.0), glm::dvec3(0.0, 1.0, 0.0), 36.0, 35.0, 960/4, 540/4);

	camera.sampleImage(16, scene);
	camera.saveImage("image12.tga");
	
	return 0;
}