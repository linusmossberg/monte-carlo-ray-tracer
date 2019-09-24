#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>

#include <glm/vec3.hpp>

#include "Random.h"
#include "Util.h"
#include "Scene.h"
#include "Camera.h"

int main()
{
	unsigned seed = std::random_device{}();
	seed = 638719719;
	std::cout << "Seed: " << seed << std::endl;
	Random::seed(seed);

	Material red(glm::pow(glm::dvec3(1.0, 0.5, 0.5), glm::dvec3(2.2)), glm::dvec3(0.0));
	Material gray(glm::dvec3(0.7), glm::dvec3(0.0));
	Material black(glm::dvec3(0.0), glm::dvec3(0.0));
	Material blue(glm::pow(glm::dvec3(0.5, 0.882, 1.0), glm::dvec3(2.2)), glm::dvec3(0.0));
	Material green(glm::pow(glm::dvec3(0.5, 1.0, 0.5), glm::dvec3(2.2)), glm::dvec3(0.0), Material::ORENNAYAR);
	Material specular(glm::dvec3(0.99), glm::dvec3(0.0), Material::Type::SPECULAR);
	Material light(glm::dvec3(0.8), glm::dvec3(4*33.333));
	Material light2(glm::dvec3(0.8), 30.0 * glm::pow(glm::dvec3(1.000, 0.973, 0.788), glm::dvec3(4.0)));
	Material light3(glm::dvec3(0.8), 30.0 * glm::pow(glm::dvec3(0.890, 0.961, 1.000), glm::dvec3(4.0)));
	
	Scene scene;
	scene.surfaces.resize(16);
	std::generate(scene.surfaces.begin(), scene.surfaces.end(), [&]() {
		return std::make_shared<Surface::Sphere>(glm::dvec3(rnd(-.6, .6), rnd(-.35, .35), rnd(-1., -4.5)), rnd(.05, .15));
	});

	scene.surfaces[3]->material = std::make_shared<Material>(light2);
	scene.surfaces[8]->material = std::make_shared<Material>(specular);

	scene.surfaces[4]->material = std::make_shared<Material>(specular);
	scene.surfaces[1]->material = std::make_shared<Material>(specular);
	scene.surfaces[11]->material = std::make_shared<Material>(specular);

	double a = 1.0, b = 0.7, c = -5, d = 10;
	glm::dvec3 v0(-a, -b, c), v1(a, b, c), v2(-a, b, c), v3(a, -b, c);
	glm::dvec3 v4(-a, -b, d), v5(a, -b, d), v6(a, b, d), v7(-a, b, d);

	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v1, v2, gray));	// Back
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v3, v1, gray));
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v4, v5, gray));	// Bottom
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v5, v3, gray));
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v7, v4, red));		// Left
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v0, v2, v7, red));
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v1, v7, v2, gray));	// Top
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v1, v6, v7, gray));
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v1, v3, v5, blue));	// Right
	scene.surfaces.push_back(std::make_shared<Surface::Triangle>(v1, v5, v6, blue));

	scene.surfaces.push_back(std::make_shared<Surface::Sphere>(glm::dvec3(-.15-0.15, -.55, -3.0), 0.15, blue));
	scene.surfaces.push_back(std::make_shared<Surface::Sphere>(glm::dvec3(.15-0.15, -.55, -3.0), 0.15, red));
	scene.surfaces.push_back(std::make_shared<Surface::Sphere>(glm::dvec3(.0-0.15, -.55, -3.0 + 3.0*sqrt(3.0)/20.0), 0.15, green));
	scene.surfaces.push_back(std::make_shared<Surface::Sphere>(glm::dvec3(.0, .0, -2.5), 0.1/2, light));

	Camera camera(glm::dvec3(0.0, -0.16, 0.7), glm::dvec3(0.0, 0.0, -1.0), glm::dvec3(0.0, 1.0, 0.0), 43.0, 35.0, 960, 540);
	camera.setPosition(glm::dvec3(1.0, 0.2, -2.7));
	camera.lookAt(glm::dvec3(-.15, -.55-0.025, -3.0 + tan(M_PI / 6.0)*0.15));

	scene.surfaces.erase(scene.surfaces.begin() + 9);
	scene.surfaces.erase(scene.surfaces.begin() + 0);

	scene.findEmissive();

	camera.sampleImage(6, scene);
	camera.saveImage("OREN-NAYAR_0-5_9");
	
	return 0;
}