/*************************************************
Simple class to abstract away some stuff from main.
**************************************************/

#pragma once

#include "Scene.hpp"
#include "../camera/camera.hpp"
#include "../photon-map/photon-map.hpp"

struct SceneRenderer
{
    SceneRenderer(std::shared_ptr<Camera> c, std::shared_ptr<Scene> s, std::shared_ptr<PhotonMap> p)
        : camera(c), scene(s), photon_map(p) { }

    void render()
    {
        std::cout << std::string(28, '-') << "| MAIN RENDERING PASS |" << std::string(28, '-') << std::endl << std::endl;
        std::cout << "Samples per pixel: " << pow2(static_cast<double>(camera->sqrtspp)) << std::endl << std::endl;
        auto before = std::chrono::system_clock::now();
        camera->sampleImage(scene, photon_map);
        camera->saveImage();
        auto now = std::chrono::system_clock::now();
        std::cout << "\r" + std::string(100, ' ') + "\r";
        std::cout << "Render Completed: " << Format::date(now);
        std::cout << ", Elapsed Time: " << Format::timeDuration(std::chrono::duration_cast<std::chrono::milliseconds>(now - before).count()) << std::endl;
    }

    void testPhotonMap()
    {
        std::ofstream log("photon_map_test_5M.csv");
        log << "max_node_data, mem_usage_GB, find_msec" << std::endl;

        size_t end_node_data = 2000;
        size_t num_iterations = 20000;

        size_t photon_emissions = size_t(5e6);
        double caustic_factor = 1.0;
        double radius = 0.1;
        double caustic_radius = 0.1;

        for (uint16_t max_node_data = 1; max_node_data < end_node_data; max_node_data++)
        {
            PhotonMap photon_map_test(scene, photon_emissions, max_node_data, caustic_factor, radius, caustic_radius, false, false);
            photon_map_test.test(log, num_iterations);
        }
        log.close();
    }

    std::shared_ptr<Camera> camera;
    std::shared_ptr<PhotonMap> photon_map;
    std::shared_ptr<Scene> scene;
};
