#include <filesystem>
#include <iostream>
#include <fstream>

#include "camera/camera.hpp"

#include "common/option.hpp"
#include "random/random.hpp"
#include "common/util.hpp"

int main()
{
    Random::seed(std::random_device{}());
    
    std::filesystem::path path(std::filesystem::current_path() / "scenes");
    std::cout << "Scene directory:" << std::endl << path.string() << std::endl << std::endl;

    std::vector<Option> options;
    try
    {
        options = availible(path);
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        waitForInput();
        return -1;
    }

    if (options.size() == 0)
    {
        std::cout << "No scenes found." << std::endl;
        waitForInput();
        return -1;
    }

    Option scene_option = getOption(options);

    {
        std::ifstream scene_file(scene_option.path);
        nlohmann::json j;
        scene_file >> j;
        scene_file.close();

        std::unique_ptr<Camera> camera;
        try
        {
            camera = std::make_unique<Camera>(j, scene_option);
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            waitForInput();
            return -1;
        }

        camera->capture();
    }

    waitForInput();
    
    return 0;
}
