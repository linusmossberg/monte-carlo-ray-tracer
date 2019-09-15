#pragma once

#include <vector>
#include <memory>

#include "Surface.h"

struct Scene
{
	std::shared_ptr<Surface::Base> light = std::make_shared<Surface::Sphere>(glm::dvec3(.0, .55, -2.5), 0.1);
	std::vector<std::shared_ptr<Surface::Base>> surfaces;
};