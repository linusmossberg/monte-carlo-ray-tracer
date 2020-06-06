#pragma once

#include <string>
#include <filesystem>
#include <vector>

struct Option
{
    Option(const std::filesystem::path& path, const std::string& camera, int camera_idx, bool photon_map)
        : path(path), camera(camera), camera_idx(camera_idx), photon_map(photon_map){ }

    std::filesystem::path path;
    std::string camera;
    int camera_idx;
    bool photon_map;
};

std::vector<Option> availible(std::filesystem::path path);

Option getOption(std::vector<Option>& options);
