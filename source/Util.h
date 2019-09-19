#pragma once

#include <random>
#include <cstdlib>
#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Random.h"

static double rnd(const double& v1, const double& v2)
{
	return Random::range(v1, v2);
}

static double mm2m(const double& mm)
{
	return mm / 1000.0;
}

static glm::dvec3 orthogonal_unit_vector(const glm::dvec3& v)
{
	if (abs(v.x) + abs(v.y) < 0.0000001)
	{
		return glm::dvec3(0.0, 1.0, 0.0);
	}
	return glm::normalize(glm::dvec3(-v.y, v.x, 0.0));
}
