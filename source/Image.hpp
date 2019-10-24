#pragma once

#include <vector>
#include <cstdint>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/component_wise.hpp>

#include "PixelOperators.hpp"

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

struct Image
{
    Image(size_t width, size_t height);

    void save(const std::string& filename) const;

    glm::dvec3& operator()(size_t col, size_t row);

    size_t width, height;
    size_t num_pixels;

private:
    double getMid() const;
    double getMax() const;
    double getExposureFactor() const;

    std::vector<glm::dvec3> blob;
};
