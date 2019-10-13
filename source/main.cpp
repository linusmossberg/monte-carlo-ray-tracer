#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <filesystem>

#include <fstream>
#include <sstream>

#include <glm/vec3.hpp>

#include "SceneParser.hpp"
#include "Random.hpp"
#include "Util.hpp"
#include "Scene.hpp"
#include "Camera.hpp"

int main()
{
    Random::seed(std::random_device{}());

    std::filesystem::path path(std::filesystem::current_path().string() + "\\scenes");
    std::cout << "Scene directory:" << std::endl << path.string() << std::endl << std::endl;

    std::vector<std::pair<std::filesystem::path, int>> options;

    try
    {
        options = SceneParser::availible(path);
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

    size_t scene_option = getSceneOption(options);

    std::string file = options[scene_option].first.filename().string();
    file.erase(file.find("."), file.length());
    std::cout << "Scene file " << file << " with camera " << options[scene_option].second << " selected." << std::endl << std::endl;
    
    std::unique_ptr<CameraScenePair> csp;
    try
    {
        csp = std::make_unique<CameraScenePair>(SceneParser::parseScene(options[scene_option].first, options[scene_option].second));
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        waitForInput();
        return -1;
    }

    auto before = std::chrono::system_clock::now();

    csp->camera->sampleImage(csp->scene);
    csp->camera->saveImage(csp->scene.savename);

    auto now = std::chrono::system_clock::now();

    std::cout << std::endl << std::endl << "Render Completed: " << formatDate(now);
    std::cout << ", Elapsed Time: " << formatTimeDuration(std::chrono::duration_cast<std::chrono::milliseconds>(now - before).count()) << std::endl;

    waitForInput();
    
    return 0;
}