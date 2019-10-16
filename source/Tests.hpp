#pragma once

#include <iostream>
#include <fstream>

/*WINDOWS*/
#include "windows.h"
#include "psapi.h"
/*WINDOWS*/

#include "Scene.hpp"
#include "PhotonMap.hpp"

inline void testPhotonMap(Scene& s, size_t photon_emissions, size_t num_radius_searches, uint16_t start_node_points, uint16_t end_node_points, uint16_t node_points_step)
{
    std::ofstream log("photon_map_log.csv");
    std::cout << "max_node_points, build_msec, find_msec, count, mem_usage_MB" << std::endl;
    log << "max_node_points, build_msec, find_msec, count, mem_usage_MB" << std::endl;
    for (uint16_t max_node_points = start_node_points; max_node_points <= end_node_points; max_node_points += node_points_step)
    {
        auto build_before = std::chrono::system_clock::now();
        PhotonMap test(s, photon_emissions, max_node_points);
        auto build_now = std::chrono::system_clock::now();

        glm::vec3 p(5.0f, -5.0f, 0.0);
        size_t count = 0;
        auto before = std::chrono::system_clock::now();
        for (size_t i = 0; i < num_radius_searches; i++)
        {
            std::vector<std::shared_ptr<OctreeData>> results;
            test.global.dataInRadius(p, 0.1f, results);
            count = results.size();
        }
        auto now = std::chrono::system_clock::now();

        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)& pmc, sizeof(pmc));
        SIZE_T vmem_used = pmc.PrivateUsage;

        std::cout << max_node_points << ", ";
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(build_now - build_before).count() << ", ";
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(now - before).count() << ", ";
        std::cout << count << ", ";
        std::cout << vmem_used / 1e6 << std::endl;

        log << max_node_points << ", ";
        log << std::chrono::duration_cast<std::chrono::milliseconds>(build_now - build_before).count() << ", ";
        log << std::chrono::duration_cast<std::chrono::milliseconds>(now - before).count() << ", ";
        log << count << ", ";
        log << vmem_used / 1e6 << std::endl;
    }
    log.close();
}
