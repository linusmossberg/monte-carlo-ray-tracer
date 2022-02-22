#pragma once

#include <queue>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <nlohmann/json.hpp>

inline std::ostream& operator<<(std::ostream& out, const glm::dvec3& v)
{
    return out << std::string("( " + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + " )");
}

inline glm::dvec3 intToColor(uint32_t i)
{
    return glm::dvec3((i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF) / 255.0;
}

void Log(const std::string& message);

void waitForInput();

struct Transform
{
    Transform(const glm::dvec3 &position, const glm::dvec3 &scale, const glm::dvec3 &rotation);

    glm::dvec3 transformNormal(const glm::dvec3& normal) const;

    glm::dmat4 matrix, rotation_matrix;
    const glm::dvec3 position, scale, rotation;
    bool negative_determinant;
};

namespace glm
{
    void from_json(const nlohmann::json &j, dvec3 &v);
}

template <class T>
inline T getOptional(const nlohmann::json &j, std::string field, T default_value)
{
    T ret = default_value;
    if (j.find(field) != j.end())
    {
        ret = j.at(field).get<T>();
    }
    return ret;
}

template <class T>
inline void getToOptional(const nlohmann::json &j, std::string field, T& value)
{
    if (j.find(field) != j.end())
    {
        j.at(field).get_to(value);
    }
}

inline bool solveQuadratic(double a, double b, double c, double& t_min, double& t_max)
{
    if (a != 0.0)
    {
        double d = b * b - 4.0 * a * c;

        if (d < 0.0) return false;

        double t = -0.5 * (b + (b < 0.0 ? -std::sqrt(d) : std::sqrt(d)));

        t_min = t / a;
        t_max = c / t;

        if (t_min > t_max) std::swap(t_min, t_max);

        return true;
    }
    if (b != 0.0)
    {
        t_min = t_max = -c / b;
        return true;
    }
    return false;
}

inline double powerHeuristic(double a_pdf, double b_pdf)
{
    double a_pdf2 = a_pdf * a_pdf;
    return a_pdf2 / (a_pdf2 + b_pdf * b_pdf);
}

template<class T>
inline std::priority_queue<T> reservedPriorityQueue(size_t size)
{
    std::vector<T> container; container.reserve(size);
    return std::priority_queue<T, std::vector<T>, std::less<T>>(std::less<T>(), std::move(container));
}

template<class T>
struct AccessiblePQ : public std::priority_queue<T, std::vector<T>, std::less<T>>
{
    void clear() { this->c.clear(); }
    const std::vector<T>& container() const { return this->c; }
};