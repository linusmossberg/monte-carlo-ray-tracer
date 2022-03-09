#pragma once

#include <atomic>
#include <vector>
#include <functional>

#include <glm/glm.hpp>

#include <nlohmann/json.hpp>

class Film
{
    typedef glm::vec<2, int64_t> ivec2;

public:
    Film();

    Film(size_t width, size_t height);

    Film(size_t width, size_t height, const nlohmann::json& j);

    void deposit(const glm::dvec2& p, const glm::dvec3& v);

    glm::dvec3 scan(size_t col, size_t row) const;

private:
    double filter(double x) const;

    struct Splat
    {
        void update(const glm::dvec3& v, double weight);

        glm::dvec3 get() const;

    private:
        std::atomic<double> rgb_sum[3];
        std::atomic<double> weight_sum;
    };

    std::vector<Splat> blob;

    std::vector<double> filter_cache;

    double radius;
    double two_inv_radius;
    double inv_dx;

    size_t width, height;

    std::function<double(double)> filter_function;
};