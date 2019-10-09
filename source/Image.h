#pragma once

#include <vector>
#include <cstdint>
#include <fstream>

//Hard coded (except for dimensions) uncompressed 24bpp true-color TGA header.
//After writing this to file, the RGB bytes can be dumped in sequence (left to right, top to bottom) to create a TGA image.
struct HeaderTGA
{
	HeaderTGA(uint16_t width, uint16_t height)
		: width(width), height(height) {}

private:
	uint8_t begin[12] = { 0, 0, 2 };
	uint16_t width;
	uint16_t height;
	uint8_t end[2] = { 24, 32 };
};

inline glm::dvec3 filmic(glm::dvec3 in)
{
	const double A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30, W = 11.2;

	auto Uncharted2Tonemap = [&](const glm::dvec3& x)
	{
		return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
	};

	in *= 3.0;

	return Uncharted2Tonemap(in) / Uncharted2Tonemap(glm::dvec3(W));
}

inline glm::dvec3 reinhard(glm::dvec3 in)
{
	in *= 1.5;
	return in / (1.0 + in);
}

inline glm::dvec3 adjustments(glm::dvec3 in, double midpoint)
{
	//double contrast = 1.05;
	//return contrast * (in - glm::dvec3(midpoint)) + glm::dvec3(midpoint);
	return in;
}

inline glm::dvec3 gammaCorrect(glm::dvec3 in)
{
	return glm::pow(in, glm::dvec3(1.0 / 2.2));
}

inline std::vector<uint8_t> truncate(glm::dvec3 in)
{
	in = glm::clamp(in, glm::dvec3(0.0), glm::dvec3(1.0)) * std::nextafter(256.0, 0.0);
	return { (uint8_t)in.b, (uint8_t)in.g, (uint8_t)in.r };
}

struct Image
{
	Image(size_t width, size_t height)
		: blob(std::vector<glm::dvec3>((uint64_t)width* height, glm::dvec3())), width(width), height(height) { }

	void save(const std::string& filename) const
	{
		double midpoint = getMidpoint();

		HeaderTGA header((uint16_t)width, (uint16_t)height);
		std::ofstream out_tonemapped(filename + ".tga", std::ios::binary);
		out_tonemapped.write(reinterpret_cast<char*>(&header), sizeof(header));
		for (const auto& p : blob)
		{
			std::vector<uint8_t> fp = truncate(gammaCorrect(adjustments(filmic(p), midpoint)));
			out_tonemapped.write(reinterpret_cast<char*>(fp.data()), fp.size() * sizeof(uint8_t));
		}
		out_tonemapped.close();
	}

	glm::dvec3& operator()(size_t col, size_t row)
	{
		return blob[row * width + col];
	}

	double getMidpoint() const
	{
		double sum = 0.0;
		for (const auto& p : blob)
		{
			glm::dvec3 fp = glm::clamp(filmic(p), glm::dvec3(0.0), glm::dvec3(1.0));
			sum += (fp.x + fp.y + fp.z) / 3.0;
		}
		return sum / blob.size();
	}

	size_t width, height;

private:
	std::vector<glm::dvec3> blob;
};
