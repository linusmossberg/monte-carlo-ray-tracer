#pragma once

#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include <nlohmann/json.hpp>

#include "../common/format.hpp"

inline std::ostream& operator<<(std::ostream& out, const glm::dvec3& v)
{
    return out << std::string("( " + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + " )");
}

inline double pow2(double x)
{
    return x * x;
}

inline void Log(const std::string& message)
{
    std::ofstream log("log.txt", std::ios::app);
    std::string temp = message;
    temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
    log << "[" << Format::date(std::chrono::system_clock::now()) << "] " << temp << std::endl;
}

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