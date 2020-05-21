#pragma once

#include <iostream>
#include <string>
#include <filesystem>
#include <numeric>
#include <vector>

struct SceneOption
{
    SceneOption(const std::filesystem::path& path, const std::string& camera, int camera_idx)
        : path(path), camera(camera), camera_idx(camera_idx) { }

    std::filesystem::path path;
    std::string camera;
    int camera_idx;
};

inline size_t getSceneOption(const std::vector<SceneOption>& options)
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

    return scene_option;
}

inline void waitForInput()
{
    std::cout << std::endl << "Press enter to exit." << std::flush;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    char c;
    while (std::cin.get(c) && c != '\n') {}
}
