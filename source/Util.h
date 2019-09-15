#pragma once

#include <cstdlib>
#define _USE_MATH_DEFINES
#include <math.h>

static double rnd(const double& v1, const double& v2)
{
	double min = fmin(v1, v2);
	double max = fmax(v1, v2);
	return min + ((double)rand() / RAND_MAX) * (max - min);
}

static double mm2m(const double& mm)
{
	return mm / 1000.0;
}
