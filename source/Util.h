#pragma once

#include <random>
#include <cstdlib>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iomanip>

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

static glm::dvec3 orthogonalUnitVector(const glm::dvec3& v)
{
	if (abs(v.x) + abs(v.y) < 0.0000001)
	{
		return glm::dvec3(0.0, 1.0, 0.0);
	}
	return glm::normalize(glm::dvec3(-v.y, v.x, 0.0));
}

static void writeTimeDuration(size_t msec_duration, std::ostream &out)
{
	size_t hours = msec_duration / 3600000;
	size_t minutes = (msec_duration % 3600000) / 60000;
	size_t seconds = (msec_duration % 60000) / 1000;
	size_t milliseconds = msec_duration % 1000;

	out << "Time remaining: "
		<< std::setfill('0') << std::setw(2) << hours << ":"
		<< std::setfill('0') << std::setw(2) << minutes << ":"
		<< std::setfill('0') << std::setw(2) << seconds << "."
		<< std::setfill('0') << std::setw(3) << milliseconds << std::endl;
}
