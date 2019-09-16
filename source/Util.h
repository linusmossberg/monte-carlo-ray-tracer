#pragma once

#include <random>
#include <cstdlib>
#define _USE_MATH_DEFINES
#include <math.h>

#include "Random.h"

static double rnd(const double& v1, const double& v2)
{
	return Random::range(v1, v2);
}

static double mm2m(const double& mm)
{
	return mm / 1000.0;
}
