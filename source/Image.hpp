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

// Tone mapping operator developed by John Hable for Uncharted 2
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
inline glm::dvec3 filmic(glm::dvec3 in)
{
    const double A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30, W = 11.2;

    auto f = [&](const glm::dvec3& x)
    {
        return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
    };

    // exposure factor to normalize overall image intensity across tonemapping operators
    in *= 3.0;

    return f(in) / f(glm::dvec3(W));
}

inline glm::dvec3 reinhard(glm::dvec3 in)
{
    // exposure factor to normalize overall image intensity across tonemapping operators
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
        : blob(std::vector<glm::dvec3>((uint64_t)width* height, glm::dvec3())), width(width), height(height), num_pixels(width*height) { }

    void save(const std::string& filename) const
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

    glm::dvec3& operator()(size_t col, size_t row)
    {
        return blob[row * width + col];
    }

    double getMid() const
    {
        double sum = 0.0;
        for (const auto& p : blob)
        {
            glm::dvec3 fp = glm::clamp(filmic(p), glm::dvec3(0.0), glm::dvec3(1.0));
            sum += (fp.x + fp.y + fp.z) / 3.0;
        }
        return sum / blob.size();
    }

    double getMax() const
    {
        double max = 0.0;
        for (const auto& p : blob)
        {
            double t = (p.x + p.y + p.z) / 3.0;
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
    double getExposureFactor() const
    {
        std::vector<size_t> histogram(65536, 0);
        double bin_size = getMax() / histogram.size();

        for (const auto& p : blob)
        {
            double t = (p.x + p.y + p.z) / 3.0;
            histogram[static_cast<size_t>(std::floor(t / bin_size))]++;
        }

        size_t clip_num_pixels = static_cast<size_t>(blob.size() * 0.15);
        size_t count = 0;
        double L = 1.0;
        for (size_t i = histogram.size() - 1; i  >= 0; i--)
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

    size_t width, height;
    size_t num_pixels;
private:
    std::vector<glm::dvec3> blob;
};
