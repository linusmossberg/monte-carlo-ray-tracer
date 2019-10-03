#pragma once

#include <random>
#include <cstdlib>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Random.h"

inline double rnd(double v1, double v2)
{
	return Random::range(v1, v2);
}

inline double mm2m(double mm)
{
	return mm / 1000.0;
}

inline void writeTimeDuration(size_t msec_duration, size_t thread, std::ostream &out)
{
	size_t hours = msec_duration / 3600000;
	size_t minutes = (msec_duration % 3600000) / 60000;
	size_t seconds = (msec_duration % 60000) / 1000;
	size_t milliseconds = msec_duration % 1000;

	// Create string first to avoid jumbled output when multiple threads write simultaneously
	std::stringstream ss;
	ss	<< "\rTime remaining: "
		<< std::setfill('0') << std::setw(2) << hours << ":"
		<< std::setfill('0') << std::setw(2) << minutes << ":"
		<< std::setfill('0') << std::setw(2) << seconds << "."
		<< std::setfill('0') << std::setw(3) << milliseconds << " (thread " << thread << ")";

	out << ss.str();
}

inline size_t printSceneOptionTable(const std::vector<std::pair<std::filesystem::path, int>> &options)
{
	size_t max_opt = 13, max_fil = 0, max_cam = 7;
	for (const auto& o : options)
	{
		std::string file = o.first.filename().string();
		file.erase(file.find("."), file.length());

		if (file.size() > max_fil)
			max_fil = file.size();
	}
	max_fil++;

	std::cout << " " << std::string(max_opt + max_fil + max_cam + 5, '_') << std::endl;

	auto printLine = [](std::vector<std::pair<std::string, int>> line) {
		std::cout << "| ";
		for (const auto& l : line)
		{
			std::cout << std::left << std::setw(l.second) << l.first;
			std::cout << "| ";
		}
		std::cout << std::endl;
	};

	printLine({ { "Scene option", max_opt }, { "File", max_fil }, { "Camera", max_cam } });

	std::string sep("|" + std::string(max_opt + 1, '_') + '|' + std::string(max_fil + 1, '_') + '|' + std::string(max_cam + 1, '_') + '|');
	std::cout << sep << std::endl;

	for (int i = 0; i < options.size(); i++)
	{
		std::string file = options[i].first.filename().string();
		file.erase(file.find("."), file.length());

		printLine({ {std::to_string(i), max_opt},{file, max_fil},{std::to_string(options[i].second), max_cam} });
		std::cout << sep << std::endl;
	}

	int scene_option;
	std::cout << std::endl << "Select scene option: ";
	while (std::cin >> scene_option)
	{
		if (scene_option < 0 || scene_option >= options.size())
			std::cout << "Invalid scene number, try again: ";
		else
			break;
	}
	return scene_option;
}

inline double pow2(double x)
{
	return x * x;
}

//inline glm::dvec3 localToGlobalUnitVector(const glm::dvec3& V, const glm::dvec3& N)
//{
//	glm::dvec3 X, Y;
//
//	//if (abs(N.x) > abs(N.y))
//	//	X = glm::dvec3(-N.z, 0, N.x) / sqrt(pow2(N.x) + pow2(N.z));
//	//else
//	//	X = glm::dvec3(0, N.z, -N.y) / sqrt(pow2(N.y) + pow2(N.z));
//
//	X = orthogonalUnitVector(N);
//
//	Y = glm::cross(N, X);
//
//	return glm::normalize(X*V.x + Y * V.y + N * V.z);
//}

inline glm::dvec3 orthogonalUnitVector(const glm::dvec3& v)
{
	if (abs(v.x) > abs(v.y))
		return glm::dvec3(-v.z, 0, v.x) / sqrt(pow2(v.x) + pow2(v.z));
	else
		return glm::dvec3(0, v.z, -v.y) / sqrt(pow2(v.y) + pow2(v.z));
}

struct CoordinateSystem
{
	CoordinateSystem(const glm::dvec3& N)
	{
		glm::dvec3 X = orthogonalUnitVector(N);
		T = glm::dmat3(X, glm::cross(N, X), N);
	}

	glm::dvec3 localToGlobal(const glm::dvec3& v) const
	{
		return glm::normalize(T * v);
	}

	glm::dvec3 globalToLocal(const glm::dvec3& v) const
	{
		return glm::normalize(glm::inverse(T) * v);
	}

	static glm::dvec3 localToGlobalUnitVector(const glm::dvec3& v, const glm::dvec3& N)
	{
		glm::dvec3 tX = orthogonalUnitVector(N);
		glm::dvec3 tY = glm::cross(N, tX);

		return glm::normalize(tX * v.x + tY * v.y + N * v.z);
	}

	glm::dmat3 T;
};


