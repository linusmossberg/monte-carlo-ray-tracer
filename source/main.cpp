#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <filesystem>

#include <fstream>
#include <sstream>

/**/
#include "windows.h"
#include "psapi.h"
/**/

#include <glm/vec3.hpp>

#include <nanoflann.hpp>

#include "SceneParser.hpp"
#include "Random.hpp"
#include "Util.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "PhotonMap.hpp"

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

    std::cout << "max_points, build_msec, find_msec, count, control_count, mem_usage_MB" << std::endl;
    for (uint16_t max_points = 1; max_points <= 100; max_points += 3)
    {
        auto build_before = std::chrono::system_clock::now();
        PhotonMap test(csp->scene, max_points);
        auto build_now = std::chrono::system_clock::now();

        glm::vec3 p(5.0f, -5.0f, 0.0);
        size_t ps = 100000;
        size_t count = 0;
        auto before = std::chrono::system_clock::now();
        for (size_t i = 0; i < ps; i++)
        {
            std::vector<OctreeData*> results;
            test.global.getPointsInRadius(p, 0.1f, results);
            count = results.size();
        }
        auto now = std::chrono::system_clock::now();

        size_t check_count = 0;
        for (const auto& pt : test.global_vec)
        {
            if (glm::distance(pt.pos(), p) <= 0.1f)
            {
                check_count++;
            }
        }

        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)& pmc, sizeof(pmc));
        SIZE_T vmem_used = pmc.PrivateUsage;

        std::cout << max_points << ", ";
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(build_now - build_before).count() << ", ";
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(now - before).count() << ", ";
        std::cout << count << ", ";
        std::cout << check_count << ", ";
        std::cout << vmem_used / 1e6 << std::endl;
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