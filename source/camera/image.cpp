#include "image.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include "pixel-operators.hpp"
#include "../common/util.hpp"

Image::Image(const nlohmann::json &j)
{
    width = j.at("width");
    height = j.at("height");
    num_pixels = width * height;
    blob = std::vector<glm::dvec3>(num_pixels, glm::dvec3());

    double pre_EV = getOptional(j, "pre_exposure_compensation", 0.0);
    double post_EV = getOptional(j, "post_exposure_compensation", 0.0);

    pre_scale = std::pow(2, pre_EV);
    post_scale = std::pow(2, post_EV);
}

void Image::save(const std::string& filename) const
{
    double exposure_factor = getExposureFactor() * pre_scale;
    double post_exposure_factor = getExposureFactor(exposure_factor) * post_scale;

    HeaderTGA header((uint16_t)width, (uint16_t)height);
    std::ofstream out_tonemapped(filename + ".tga", std::ios::binary);
    out_tonemapped.write(reinterpret_cast<char*>(&header), sizeof(header));
    for (const auto& p : blob)
    {
        std::vector<uint8_t> fp = truncate(gammaCorrect(filmic(p * exposure_factor) * post_exposure_factor));
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
50% of the pixels be < 0.5 and 50% of the pixels be > 0.5. If pre_exposure_factor is set to a positive 
value, then the post exposure factor is found instead.

Conceptually, the pre exposure factor corresponds to adjusting the aperture and exposure time to 
control the amount of light reaching the film/sensor, while the post exposure factor corresponds 
to the gain applied to the resulting image intensity values afterwards (after tone-mapping/camera response).
********************************************************************************************/
double Image::getExposureFactor(double pre_exposure_factor) const
{
    auto get_t = [&](const glm::dvec3 &p)
    {
        double t;
        if (pre_exposure_factor > 0.0)
            t = glm::compMax(filmic(p * pre_exposure_factor));
        else
            t = glm::compMax(p);

        return t;
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

    size_t half_of_pixels = static_cast<size_t>(blob.size() * 0.50);
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