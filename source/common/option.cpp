#include "option.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

#include "util.hpp"

std::vector<Option> availible(std::filesystem::path path)
{
    std::vector<Option> options;
    for (const auto& file : std::filesystem::directory_iterator(path))
    {
        if (!file.path().has_extension() || file.path().extension() != ".json")
            continue;

        std::ifstream scene_file(file.path());
        nlohmann::json j;
        scene_file >> j;

        bool photon_map = j.find("photon_map") != j.end();

        int i = 0;
        for (const auto& c : j.at("cameras"))
        {
            glm::dvec3 eye = c.at("eye");
            double f = c.at("focal_length");
            double s = c.at("sensor_width");
            std::stringstream ss;
            ss << "Eye: " << std::fixed << std::setprecision(0) << "(" << eye.x << " " << eye.y << " " << eye.z << "), ";
            ss << "Focal length: " << int(f) << "mm (" << int(s) << "mm)";
            options.emplace_back(file.path(), ss.str(), i, photon_map);
            i++;
        }
        scene_file.close();
    }
    return options;
}

Option getOption(std::vector<Option>& options)
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

    printLine({ { "Option", max_opt }, { "File", max_fil }, { "Camera", max_cam } });

    std::string sep("|" + std::string(max_opt + 1, '_') + '|' + std::string(max_fil + 1, '_') + '|' + std::string(max_cam + 1, '_') + '|');
    std::cout << sep << std::endl;

    for (int i = 0; i < options.size(); i++)
    {
        std::string file = options[i].path.filename().string();
        file.erase(file.find("."), file.length());

        printLine({ {std::to_string(i), max_opt},{file, max_fil},{options[i].camera, max_cam} });
        std::cout << sep << std::endl;
    }

    int option;
    std::cout << std::endl << "Select option: ";
    while (std::cin >> option)
    {
        if (option < 0 || option >= options.size())
            std::cout << "Invalid option, try again: ";
        else
            break;
    }

    if (options[option].photon_map)
    {
        char a;
        std::cout << "\nUse photon mapping? (y/n) ";
        while (std::cin >> a)
        {
            if (a == 'y' || a == 'Y' || a == 'n' || a == 'N') break;
            std::cout << "Answer with the letters y or n: ";
        }
        std::cout << std::endl;
        if (a == 'n' || a == 'N')
        {
            options[option].photon_map = false;
        }
    }

    return options[option];
}