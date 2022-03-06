#pragma once

#include <algorithm>
#include <string>

#include "film.hpp"

#include "filter.hpp"
#include "../common/util.hpp"

Film::Film() { }

Film::Film(size_t width, size_t height)
    : width(width), height(height), blob(width* height), 
      filter_function(Filter::box), radius(0.5), 
      two_inv_radius(2.0 / radius) 
{ }

Film::Film(size_t width, size_t height, const nlohmann::json& j)
    : width(width), height(height), blob(width* height)
{
    std::string filter_type = j.at("filter");
    std::transform(filter_type.begin(), filter_type.end(), filter_type.begin(), tolower);

    radius = getOptional(j, "radius", 2.0);

    if (filter_type == "mitchell-netravali")
        filter_function = Filter::MitchellNetravali<>;
    else if (filter_type == "catmull-rom")
        filter_function = Filter::CatmullRom;
    else if (filter_type == "b-spline")
        filter_function = Filter::BSpline;
    else if (filter_type == "hermite")
        filter_function = Filter::Hermite;
    else if (filter_type == "gaussian")
        filter_function = Filter::Gaussian;
    else if (filter_type == "lanczos")
        filter_function = Filter::Lanczos;
    else
    {
        filter_function = Filter::box;
        radius = getOptional(j, "radius", 0.5);
    }
    two_inv_radius = 2.0 / radius;
}

void Film::deposit(const glm::dvec2& p, const glm::dvec3& v)
{
    ivec2 min = glm::max(ivec2(p + 0.5 - radius), ivec2(0));
    ivec2 max = glm::min(ivec2(p - 0.5 + radius), ivec2(width - 1, height - 1));

    // Lazy but general and about as fast as can be
    thread_local std::vector<double> weights_x; weights_x.clear();
    for (int64_t x = min.x; x <= max.x; x++)
        weights_x.push_back(filter(x + 0.5 - p.x));

    for (int64_t y = min.y; y <= max.y; y++)
    {
        double weight_y = filter(y + 0.5 - p.y);
        for (int64_t x = min.x; x <= max.x; x++)
        {
            blob[y * width + x].update(v, weight_y * weights_x[x - min.x]);
        }
    }
}

glm::dvec3 Film::scan(size_t col, size_t row) const
{
    return blob[row * width + col].get();
}

double Film::filter(double x) const
{
    return filter_function(two_inv_radius * std::abs(x));
}

void Film::Splat::update(const glm::dvec3& v, double weight)
{
    rgb_sum[0] += v[0] * weight;
    rgb_sum[1] += v[1] * weight;
    rgb_sum[2] += v[2] * weight;
    weight_sum += weight;
}

glm::dvec3 Film::Splat::get() const
{
    glm::dvec3 res(rgb_sum[0].load(), rgb_sum[1].load(), rgb_sum[2].load());
    double w = weight_sum.load();
    if (w == 0.0) return glm::dvec3(0.0);
    return glm::max(res / w, 0.0);
}