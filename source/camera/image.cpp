#include "image.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include "pixel-operators.hpp"
#include "../common/util.hpp"
#include "../color/srgb.hpp"

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
        std::vector<uint8_t> fp = truncate(sRGB::gammaCompress(tonemap(p * exposure_factor) * gain_factor));
        out_tonemapped.write(reinterpret_cast<char*>(fp.data()), fp.size() * sizeof(uint8_t));
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
    auto get_t = [&](const glm::dvec3 &p)
    {
        return glm::compAdd(p) / 3.0;
    };

    double max = 0.0;
    for (const auto& p : blob)
    {
        double t = get_t(p);
        if (max < t) max = t;
    }

    if (max <= 0) return 1.0;

    std::vector<size_t> histogram(65536, 0);
    double bin_size = max / histogram.size();

    for (const auto& p : blob)
    {
        double t = get_t(p);
        if (t < 0) return 1.0;
        histogram[static_cast<size_t>(std::floor(t / bin_size))]++;
    }

    size_t half_of_pixels = static_cast<size_t>(blob.size() * 0.5);
    size_t count = 0;
    double L = 0.5;
    for (size_t i = 0; i < histogram.size(); i++)
    {
        count += histogram[i];
        if (count >= half_of_pixels)
        {
            L = (i + 1) * bin_size;
            break;
        }
    }
    return 0.5 / L;
}

/*************************************************************
Histogram method to find the gain that positions the histogram 
to the right such that 0.5% of image pixels are clipped
**************************************************************/
double Image::getGain(double exposure_factor) const
{
    auto get_t = [&](const glm::dvec3 &p)
    {
        return glm::compAdd(tonemap(p * exposure_factor)) / 3.0;
    };

    double max = 0.0;
    for (const auto& p : blob)
    {
        double t = get_t(p);
        if (max < t) max = t;
    }

    if (max <= 0) return 1.0;

    std::vector<size_t> histogram(65536, 0);
    double bin_size = max / histogram.size();

    for (const auto& p : blob)
    {
        double t = get_t(p);
        if (t < 0) return 1.0;
        histogram[static_cast<size_t>(std::floor(t / bin_size))]++;
    }

    size_t num_clipped_pixels = static_cast<size_t>(blob.size() * 0.01);
    size_t count = 0;
    double L = 0.99;
    for (size_t i = histogram.size() - 1; i >= 0; i--)
    {
        count += histogram[i];
        if (count >= num_clipped_pixels)
        {
            L = (i + 1) * bin_size;
            break;
        }
    }
    return 0.99 / L;
}