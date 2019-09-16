#pragma once

#include <vector>
#include <memory>

#include "Surface.h"

struct Scene
{
	std::vector<std::shared_ptr<Surface::Base>> surfaces;
};