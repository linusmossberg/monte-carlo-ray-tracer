#pragma once

#include <random>
#include <cstdlib>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <conio.h>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Random.h"

inline double rnd(double v1, double v2)
{
	return Random::range(v1, v2);
}

inline std::ostream& operator<<(std::ostream& out, const glm::dvec3& v)
{
	return out << std::string("( " + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")\n");
}

inline void waitForInput()
{
	std::cout << std::endl << "Press any key to exit." << std::endl;
	_getch();
}

inline void writeTimeDuration(size_t msec_duration, size_t thread, std::ostream &out)
{
	size_t hours = msec_duration / 3600000;
	size_t minutes = (msec_duration % 3600000) / 60000;
	size_t seconds = (msec_duration % 60000) / 1000;
	size_t milliseconds = msec_duration % 1000;

	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::milliseconds(msec_duration));
	std::string s(26, '\0');
	ctime_s(s.data(), s.size(), &now);
	s.erase(0, 4);
	s.erase(15, s.size() - 15);

	// Create string first to avoid jumbled output when multiple threads write simultaneously
	std::stringstream ss;
	ss	<< "\rTime remaining: "
		<< std::setfill('0') << std::setw(2) << hours << ":"
		<< std::setfill('0') << std::setw(2) << minutes << ":"
		<< std::setfill('0') << std::setw(2) << seconds << "."
		<< std::setfill('0') << std::setw(3) << milliseconds
		<< ", ETA: " << s;

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

	auto printLine = [](std::vector<std::pair<std::string, size_t>> line) {
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
	std::cout << std::endl;

	return scene_option;
}

inline double pow2(double x)
{
	return x * x;
}

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
		: normal(N)
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

	glm::dvec3 normal;

private:
	glm::dmat3 T;
};

//class Direction : public glm::dvec3
//{
//public:
//	Direction() 
//	{
//		x = 1;
//		y = 0;
//		z = 0;
//	};
//
//	Direction(const glm::dvec3& v) 
//	{
//		double l = glm::length(v);
//		if (l > 0)
//		{
//			x = v.x / l;
//			y = v.y / l;
//			z = v.z / l;
//		}
//		else
//		{
//			*this = Direction();
//		}
//	}
//
//	Direction(const double &v)
//	{
//		if (v != 0)
//		{
//			double d = v > 0 ? 1 / sqrt(3) : -1 / sqrt(3);
//			 x = d;
//			 y = d;
//			 z = d;
//		}
//		else
//		{
//			*this = Direction();
//		}
//	}
//
//	Direction(double vx, double vy, double vz)
//	{
//		*this = Direction(glm::dvec3(vx, vy, vz));
//	}
//
//	const Direction& operator=(const glm::dvec3& v)
//	{
//		*this = Direction(v);
//		return (const Direction&)*this;
//	}
//
//	double dot(const glm::dvec3& x, const glm::dvec3& y)
//	{
//		return glm::dot(x, y);
//	}
//
//	operator glm::dvec3() { return glm::dvec3(x, y, z); }
//};



