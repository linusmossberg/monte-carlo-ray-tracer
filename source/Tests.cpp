#pragma once

#include "Tests.hpp"

void testPhotonMap(std::shared_ptr<Scene> scene)
{
    std::ofstream log("photon_map_test_6M.csv");
    log << "max_node_data, mem_usage_GB, find_msec" << std::endl;

    size_t end_node_data = 2000;
    size_t num_iterations = 10000;

    size_t photon_emissions = size_t(1e5);
    double caustic_factor = 1.0;
    double radius = 0.1;
    double caustic_radius = 0.1;

    for (uint16_t max_node_data = 1; max_node_data < end_node_data; max_node_data++)
    {
        PhotonMap photon_map_test(scene, photon_emissions, max_node_data, caustic_factor, radius, caustic_radius, false);
        photon_map_test.test(log, num_iterations);
    }
    log.close();
}

void PhotonMap::test(std::ostream& log, size_t num_iterations) const
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)& pmc, sizeof(pmc));
    SIZE_T mem_used = pmc.WorkingSetSize;

    log << max_node_data << ", ";
    std::cout << max_node_data << ", ";
    log << mem_used / 1e9 << ", ";
    std::cout << mem_used / 1e9 << ", ";

    auto begin = std::chrono::high_resolution_clock::now();
    if (!scene->surfaces.empty())
    {
        for (int i = 0; i < num_iterations; i++)
        {
            const auto& surface = scene->surfaces[Random::uirange(0, scene->surfaces.size() - 1)]; // random surface
            glm::dvec3 point = surface->operator()(Random::range(0, 1), Random::range(0, 1)); // random point on surface

            auto caustic_photons = caustic_map.radiusSearch(point, caustic_radius);
            auto direct_photons = direct_map.radiusSearch(point, radius);
            auto indirect_photons = indirect_map.radiusSearch(point, radius);
            auto shadow_photons = shadow_map.radiusSearch(point, radius);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    log << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000.0 << std::endl;
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000.0 << std::endl;
#else
    std::cout << "Photon map testing is only supported on windows at the moment." << std::endl;
#endif
}

