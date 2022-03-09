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
      two_inv_radius(2.0 / radius), inv_dx(0.0)
{ }

Film::Film(size_t width, size_t height, const nlohmann::json& j)
    : width(width), height(height), blob(width* height)
{
    std::string filter_type = j.at("filter");
    std::transform(filter_type.begin(), filter_type.end(), filter_type.begin(), tolower);

    auto set = [&](auto f, auto r)
    {
        filter_function = f;
        radius = r;
    };

    if (filter_type == "mitchell-netravali")
        set(Filter::MitchellNetravali<>, 2.0);
    else if (filter_type == "catmull-rom")
        set(Filter::CatmullRom, 2.0);
    else if (filter_type == "b-spline")
        set(Filter::BSpline, 1.39);
    else if (filter_type == "hermite")
        set(Filter::Hermite, 1.0);
    else if (filter_type == "gaussian")
        set(Filter::Gaussian, 1.71);
    else if (filter_type == "lanczos")
        set(Filter::Lanczos, 2.0);
    else
        set(Filter::box, 0.5);

    radius = getOptional(j, "radius", radius);
    two_inv_radius = 2.0 / radius;

    if (j.find("cache_size") != j.end())
    {
        size_t cache_size = j.at("cache_size");
        filter_cache.resize(cache_size);
        for (int i = 0; i < cache_size; i++)
        {
            filter_cache[i] = filter_function((2.0 * i) / (cache_size - 1));
        }
        inv_dx = (cache_size - 1) / radius;
    }
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
    if (filter_cache.empty())
    {
        return filter_function(two_inv_radius * std::abs(x));
    }
    else
    {
        // index = round(inv_bin_size * abs(x)) = floor(inv_bin_size * abs(x) + 0.5)
        return filter_cache[static_cast<size_t>(inv_dx * std::abs(x) + 0.5)];
    }
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