#pragma once

#include <vector>
#include <memory>

#include "Surface.h"

struct Scene
{
	std::vector<std::unique_ptr<Surface::Base>> surfaces;
};