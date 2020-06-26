#pragma once

#include <vector>
#include <cstdint>
#include <functional>

#include <glm/vec3.hpp>

#include <nlohmann/json.hpp>

struct Image
{
    Image() { }
    Image(const nlohmann::json &j);

    void save(const std::string& filename) const;

    glm::dvec3& operator()(size_t col, size_t row);

    size_t width, height;
    size_t num_pixels;

private:
    double getExposure() const;
    double getGain(double exposure_factor) const;

    std::vector<glm::dvec3> blob;
    double exposure_scale, gain_scale;

    std::function<glm::dvec3(const glm::dvec3&)> tonemap;

    /**************************************************************************
    Hard coded (except for dimensions) uncompressed 24bpp true-color TGA header.
    After writing this to file, the RGB bytes can be dumped in sequence
    (left to right, top to bottom) to create a TGA image.
    ***************************************************************************/
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
};
