#pragma once

#include <random>
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

inline void waitForInput()
{
#ifdef _WIN32
    std::cout << std::endl << "Press any key to exit." << std::endl;
    _getch();
#else
    std::cout << std::endl << "Press enter to exit." << std::endl;
    std::cin.get();
#endif
}

inline std::ostream& operator<<(std::ostream& out, const glm::dvec3& v)
{
    return out << std::string("( " + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + " )");
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

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    return ss.str();
}

inline std::string formatProgress(double progress)
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

inline std::string formatLargeNumber(size_t n)
{
    std::string int_string = std::to_string(n);
    size_t pos = int_string.length() - 3;
    while (pos > 0 && pos < int_string.length())
    {
        int_string.insert(pos, " ");
        pos -= 3;
    }
    return int_string;
};

inline void printProgressInfo(double progress, size_t msec_duration, size_t sps, std::ostream &out)
{
    auto ETA = std::chrono::system_clock::now() + std::chrono::milliseconds(msec_duration);

    // Create string first to avoid jumbled output if multiple threads write simultaneously
    std::stringstream ss;
    ss << "\rTime remaining: " << formatTimeDuration(msec_duration)
       << " || " << formatProgress(progress)
       << " || ETA: " << formatDate(ETA)
       << " || Samples/s: " << formatLargeNumber(sps) + "    ";

    out << ss.str();
}

inline void Log(const std::string& message)
{
    std::ofstream log("log.txt", std::ios::app);
    std::string temp = message;
    temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
    log << "[" << formatDate(std::chrono::system_clock::now()) << "]: " << temp << std::endl;
}

struct SceneOption
{
    SceneOption(const std::filesystem::path& path, const std::string& camera, int camera_idx)
        : path(path), camera(camera), camera_idx(camera_idx) { }

    std::filesystem::path path;
    std::string camera;
    int camera_idx;
};

inline size_t getSceneOption(const std::vector<SceneOption> &options)
{
    size_t max_opt = 13, max_fil = 0, max_cam = 0;
    for (const auto& o : options)
    {
        std::string file = o.path.filename().string();
        file.erase(file.find("."), file.length());

        if (file.size() > max_fil)
            max_fil = file.size();

        if (o.camera.size() > max_cam)
            max_cam = o.camera.size();
    }
    max_fil++;
    max_cam++;

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
        std::string file = options[i].path.filename().string();
        file.erase(file.find("."), file.length());

        printLine({ {std::to_string(i), max_opt},{file, max_fil},{options[i].camera, max_cam} });
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
    //std::cout << std::endl;

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