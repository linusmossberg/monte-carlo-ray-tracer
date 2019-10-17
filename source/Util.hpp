#pragma once

#include <random>
#include <cstdlib>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <fstream>

#ifdef _WIN32
    #include <conio.h>
#endif

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

inline std::ostream& operator<<(std::ostream& out, const glm::dvec3& v)
{
    return out << std::string("( " + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")\n");
}

inline void waitForInput()
{
#ifdef _WIN32
    std::cout << std::endl << "Press any key to exit." << std::endl;
    _getch();
#elif __linux_
    std::cout << std::endl << "Press enter to exit." << std::endl;
    std::cin.get();
#else

#endif
}

inline std::string formatDate(const std::chrono::time_point<std::chrono::system_clock> &date)
{
    std::time_t now = std::chrono::system_clock::to_time_t(date);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);

    std::string s(26, ' ');
    std::strftime(s.data(), s.size(), "%Y-%m-%d %H:%M", &timeinfo);

    auto p = s.find_last_not_of(' ');
    s.erase(p, s.size() - p);

    return s;
}

inline std::string formatTimeDuration(size_t msec_duration)
{
    size_t hours = msec_duration / 3600000;
    size_t minutes = (msec_duration % 3600000) / 60000;
    size_t seconds = (msec_duration % 60000) / 1000;
    //size_t milliseconds = msec_duration % 1000;
    //seconds += size_t(std::round(milliseconds / 1000.0));

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    return ss.str();
}

inline void printProgressInfo(double progress, size_t msec_duration, size_t sps, std::ostream &out)
{
    auto formatSPS = [&sps]()
    {
        std::string int_string = std::to_string(sps);
        size_t pos = int_string.length() - 3;
        while (pos > 0 && pos < int_string.length())
        {
            int_string.insert(pos, " ");
            pos -= 3;
        }
        return int_string;
    };

    auto formatProgress = [&progress]()
    {
        std::string d_str = std::to_string(progress);
        size_t dot_pos = d_str.find('.');
        std::string l(""), r("");
        if (dot_pos != std::string::npos)
        {
            l = d_str.substr(0, dot_pos);
            r = d_str.substr(dot_pos + 1);
        }
        else
        {
            l = d_str;
        }
        size_t r_len = (4 - l.length());
        std::string r_n = r.length() >= r_len ? r.substr(0, r_len) : r + std::string(' ', static_cast<int>(r_len - r.length()));

        return l + "." + r_n + "%";
    };

    auto ETA = std::chrono::system_clock::now() + std::chrono::milliseconds(msec_duration);

    // Create string first to avoid jumbled output if multiple threads write simultaneously
    std::stringstream ss;
    ss << "\rTime remaining: " << formatTimeDuration(msec_duration)
        << " || " << formatProgress()
        << " || ETA: " << formatDate(ETA)
        << " || Samples/s: " << formatSPS() + "    ";

    out << ss.str();
}

inline void Log(const std::string& message)
{
    std::ofstream log("log.txt", std::ios::app);
    log << "[" << formatDate(std::chrono::system_clock::now()) << "]: " << message << std::endl;
}

inline size_t getSceneOption(const std::vector<std::pair<std::filesystem::path, int>> &options)
{
    size_t max_opt = 13, max_fil = 0, max_cam = 7;
    for (const auto& o : options)
    {
        std::string file = o.first.filename().string();
        file.erase(file.find("."), file.length());

        if (file.size() > max_fil)
            max_fil = file.size();
    }
    max_fil++;

    std::cout << " " << std::string(max_opt + max_fil + max_cam + 5, '_') << std::endl;

    auto printLine = [](std::vector<std::pair<std::string, size_t>> line) {
        std::cout << "| ";
        for (const auto& l : line)
        {
            std::cout << std::left << std::setw(l.second) << l.first;
            std::cout << "| ";
        }
        std::cout << std::endl;
    };

    printLine({ { "Scene option", max_opt }, { "File", max_fil }, { "Camera", max_cam } });

    std::string sep("|" + std::string(max_opt + 1, '_') + '|' + std::string(max_fil + 1, '_') + '|' + std::string(max_cam + 1, '_') + '|');
    std::cout << sep << std::endl;

    for (int i = 0; i < options.size(); i++)
    {
        std::string file = options[i].first.filename().string();
        file.erase(file.find("."), file.length());

        printLine({ {std::to_string(i), max_opt},{file, max_fil},{std::to_string(options[i].second), max_cam} });
        std::cout << sep << std::endl;
    }

    int scene_option;
    std::cout << std::endl << "Select scene option: ";
    while (std::cin >> scene_option)
    {
        if (scene_option < 0 || scene_option >= options.size())
            std::cout << "Invalid scene option, try again: ";
        else
            break;
    }
    std::cout << std::endl;

    return scene_option;
}

inline double pow2(double x)
{
    return x * x;
}

inline glm::dvec3 orthogonalUnitVector(const glm::dvec3& v)
{
    if (abs(v.x) > abs(v.y))
        return glm::dvec3(-v.z, 0, v.x) / sqrt(pow2(v.x) + pow2(v.z));
    else
        return glm::dvec3(0, v.z, -v.y) / sqrt(pow2(v.y) + pow2(v.z));
}

struct CoordinateSystem
{
    CoordinateSystem(const glm::dvec3& N)
        : normal(N)
    {
        glm::dvec3 X = orthogonalUnitVector(N);
        T = glm::dmat3(X, glm::cross(N, X), N);
    }

    glm::dvec3 localToGlobal(const glm::dvec3& v) const
    {
        return glm::normalize(T * v);
    }

    glm::dvec3 globalToLocal(const glm::dvec3& v) const
    {
        return glm::normalize(glm::inverse(T) * v);
    }

    static glm::dvec3 localToGlobalUnitVector(const glm::dvec3& v, const glm::dvec3& N)
    {
        glm::dvec3 tX = orthogonalUnitVector(N);
        glm::dvec3 tY = glm::cross(N, tX);

        return glm::normalize(tX * v.x + tY * v.y + N * v.z);
    }

    glm::dvec3 normal;

private:
    glm::dmat3 T;
};

//class Direction : public glm::dvec3
//{
//public:
//    Direction() 
//    {
//        x = 1;
//        y = 0;
//        z = 0;
//    };
//
//    Direction(const glm::dvec3& v) 
//    {
//        double l = glm::length(v);
//        if (l > 0)
//        {
//            x = v.x / l;
//            y = v.y / l;
//            z = v.z / l;
//        }
//        else
//        {
//            *this = Direction();
//        }
//    }
//
//    Direction(const double &v)
//    {
//        if (v != 0)
//        {
//            double d = v > 0 ? 1 / sqrt(3) : -1 / sqrt(3);
//             x = d;
//             y = d;
//             z = d;
//        }
//        else
//        {
//            *this = Direction();
//        }
//    }
//
//    Direction(double vx, double vy, double vz)
//    {
//        *this = Direction(glm::dvec3(vx, vy, vz));
//    }
//
//    const Direction& operator=(const glm::dvec3& v)
//    {
//        *this = Direction(v);
//        return (const Direction&)*this;
//    }
//
//    double dot(const glm::dvec3& x, const glm::dvec3& y)
//    {
//        return glm::dot(x, y);
//    }
//
//    operator glm::dvec3() { return glm::dvec3(x, y, z); }
//};



