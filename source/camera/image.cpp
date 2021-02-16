#include "image.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include "pixel-operators.hpp"
#include "../common/util.hpp"
#include "../color/srgb.hpp"
#include "../common/histogram.hpp"

Image::Image(const nlohmann::json &j)
{
    width = j.at("width");
    height = j.at("height");
    num_pixels = width * height;
    blob = std::vector<glm::dvec3>(num_pixels, glm::dvec3());

    plain = getOptional(j, "plain", false);

    double exposure_EV = getOptional(j, "exposure_compensation", 0.0);
    double gain_EV = getOptional(j, "gain_compensation", 0.0);

    exposure_scale = std::pow(2, exposure_EV);
    gain_scale = std::pow(2, gain_EV);

    std::string tonemapper = getOptional<std::string>(j, "tonemapper", "HABLE");
    std::transform(tonemapper.begin(), tonemapper.end(), tonemapper.begin(), toupper);

    if (plain)
        tonemap = linear;
    else
        if (tonemapper == "ACES")
            tonemap = filmicACES;
        else 
            tonemap = filmicHable;
}

void Image::save(const std::string& filename) const
{
    double exposure_factor = plain ? 1.0 : getExposure() * exposure_scale;
    double gain_factor = plain ? 1.0 : getGain(exposure_factor) * gain_scale;

    HeaderTGA header((uint16_t)width, (uint16_t)height);
    std::ofstream out_tonemapped(filename + ".tga", std::ios::binary);
    out_tonemapped.write(reinterpret_cast<char*>(&header), sizeof(header));
    for (const auto& p : blob)
    {
        std::vector<char> fp = truncate(sRGB::gammaCompress(tonemap(p * exposure_factor) * gain_factor));
        out_tonemapped.write(fp.data(), fp.size() * sizeof(uint8_t));
    }
    out_tonemapped.close();
}

glm::dvec3& Image::operator()(size_t col, size_t row)
{
    return blob[row * width + col];
}

/*******************************************************************************************
Histogram method to find the intensity level L that 50% of the pixels has higher/lower intensity than.
The returned exposure factor is then 0.5/L, which if multiplied by each pixel in the image will make 
50% of the pixels be < 0.5 and 50% of the pixels be > 0.5.
********************************************************************************************/
double Image::getExposure() const
{
    std::vector<double> brightness(blob.size());
    for (size_t i = 0; i < blob.size(); i++)
    {
        brightness[i] = glm::compAdd(blob[i]) / 3.0;
    }
    Histogram histogram(brightness, 65536);
    double L = histogram.level(0.5);
    return L > 0.0 ? 0.5 / L : 1.0;
}

/**************************************************************************
Histogram method to find the gain that positions the histogram to the right
***************************************************************************/
double Image::getGain(double exposure_factor) const
{
    std::vector<double> brightness(blob.size());
    for (size_t i = 0; i < blob.size(); i++)
    {
        brightness[i] = glm::compAdd(tonemap(blob[i] * exposure_factor)) / 3.0;
    }
    Histogram histogram(brightness, 65536);
    double L = histogram.level(0.99);
    return L > 0.0 ? 0.99 / L : 1.0;
}