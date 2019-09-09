#pragma once

#include <cstdlib>

float rnd(const float& v1, const float& v2)
{
	float min = fmin(v1, v2);
	float max = fmax(v1, v2);
	return min + ((float)rand() / RAND_MAX) * (max - min);
}

float mm2m(const float& mm)
{
	return mm / 1000.0f;
}
