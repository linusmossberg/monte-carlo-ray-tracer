#include "Image.hpp"

Image::Image(size_t width, size_t height)
    : blob(std::vector<glm::dvec3>((uint64_t)width* height, glm::dvec3())), 
      width(width), height(height), num_pixels(width* height) { }

void Image::save(const std::string& filename) const
{
    double midpoint = getMid();
    double exposure_factor = getExposureFactor();

    HeaderTGA header((uint16_t)width, (uint16_t)height);
    std::ofstream out_tonemapped(filename + ".tga", std::ios::binary);
    out_tonemapped.write(reinterpret_cast<char*>(&header), sizeof(header));
    for (const auto& p : blob)
    {
        std::vector<uint8_t> fp = truncate(gammaCorrect(adjustments(filmic(p * exposure_factor), midpoint)));
        out_tonemapped.write(reinterpret_cast<char*>(fp.data()), fp.size() * sizeof(uint8_t));
    }
    out_tonemapped.close();
}

glm::dvec3& Image::operator()(size_t col, size_t row)
{
    return blob[row * width + col];
}

double Image::getMid() const
{
    double sum = 0.0;
    for (const auto& p : blob)
    {
        glm::dvec3 fp = glm::clamp(filmic(p), glm::dvec3(0.0), glm::dvec3(1.0));
        sum += glm::compAdd(fp) / 3.0;
    }
    return sum / blob.size();
}

double Image::getMax() const
{
    double max = 0.0;
    for (const auto& p : blob)
    {
        double t = glm::compAdd(p) / 3.0;
        if (max < t)
        {
            max = t;
        }
    }
    return max;
}

/*******************************************************************************************
Histogram method to find the intensity level L that 85% of the pixels has less intensity than.
The returned exposure factor is then 1.0/L, which if multiplied by each pixel in the image will
make 85% of the pixels be <= 1.0 and 15% of the pixels be > 1.0 (clipped).

These pixels are not necessarily clipped in the final image however as this depends on
subsequent operations like tonemapping, which tends to bring more pixels down to <= 1.0 for
increased dynamic range.
********************************************************************************************/
double Image::getExposureFactor() const
{
    double max = getMax();
    if (max <= 0.0) return 1.0;

    std::vector<size_t> histogram(65536, 0);
    double bin_size = max / histogram.size();

    for (const auto& p : blob)
    {
        double t = glm::compAdd(p) / 3.0;
        histogram[static_cast<size_t>(std::floor(t / bin_size))]++;
    }

    size_t clip_num_pixels = static_cast<size_t>(blob.size() * 0.15);
    size_t count = 0;
    double L = 1.0;
    for (size_t i = histogram.size() - 1; i >= 0; i--)
    {
        count += histogram[i];
        if (count >= clip_num_pixels)
        {
            L = (i + 1) * bin_size;
            break;
        }
    }
    return 1.0 / L;
}