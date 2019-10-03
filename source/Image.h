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

struct Image
{
	Image(size_t width, size_t height)
		: blob(std::vector<glm::dvec3>((uint64_t)width* height, glm::dvec3())), width(width), height(height) { }

	void save(const std::string& filename) const
	{
		HeaderTGA header((uint16_t)width, (uint16_t)height);
		std::ofstream out(filename + ".tga", std::ios::binary);
		std::ofstream out_tonemapped(filename + "_tonemapped.tga", std::ios::binary);
		out.write(reinterpret_cast<char*>(&header), sizeof(header));
		out_tonemapped.write(reinterpret_cast<char*>(&header), sizeof(header));
		for (const auto& p : blob)
		{
			glm::dvec3 fp = filmic(p);
			for (int c = 2; c >= 0; c--)
			{
				double v = pow(p[c], 1.0 / 2.2);
				v = v > 1.0 ? 1.0 : v < 0.0 ? 0.0 : v;
				uint8_t pv = (uint8_t)(v * 255.0);
				out.write(reinterpret_cast<char*>(&pv), sizeof(pv));

				v = pow(fp[c], 1.0 / 2.2);
				v = v > 1.0 ? 1.0 : v < 0.0 ? 0.0 : v;
				pv = (uint8_t)(v * 255.0);
				out_tonemapped.write(reinterpret_cast<char*>(&pv), sizeof(pv));
			}
		}
		out.close();
		out_tonemapped.close();
	}

	glm::dvec3& operator()(size_t col, size_t row)
	{
		return blob[row * width + col];
	}

	size_t width, height;

private:
	std::vector<glm::dvec3> blob;
};
