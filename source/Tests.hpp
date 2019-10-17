#pragma once

#include <iostream>
#include <fstream>
#include <string>

#ifdef _WIN32
    #include "windows.h"
    #include "psapi.h"
#endif

#include "Scene.hpp"
#include "PhotonMap.hpp"

#ifdef _WIN32
inline void testPhotonMap(
    Scene& s, 
    size_t photon_emissions, 
    size_t num_radius_searches, 
    uint16_t start_node_points, 
    uint16_t end_node_points, 
    uint16_t node_points_step,
    const std::string& filename)
{
    std::ofstream log(filename);
    std::cout << "max_node_data, build_msec, find_msec, count, mem_usage_MB, destruct_msec" << std::endl;
    log << "max_node_data, build_msec, find_msec, count, mem_usage_MB, destruct_msec" << std::endl;

    auto begin = std::chrono::system_clock::now();
    auto end = std::chrono::system_clock::now();

    for (uint16_t max_node_data = start_node_points; max_node_data <= end_node_points; max_node_data += node_points_step)
    {
        if (max_node_data != start_node_points)
        {
            end = std::chrono::system_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;
            log << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;
        }

        auto build_before = std::chrono::system_clock::now();
        PhotonMap test(s, photon_emissions, max_node_data);
        auto build_now = std::chrono::system_clock::now();

        glm::vec3 p(5.0f, -5.0f, 0.0);
        size_t count = 0;
        auto before = std::chrono::system_clock::now();
        for (size_t i = 0; i < num_radius_searches; i++)
        {
            auto results = test.global.radiusSearch(p, 0.1f);
            count = results.size();
        }
        auto now = std::chrono::system_clock::now();

        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)& pmc, sizeof(pmc));
        SIZE_T vmem_used = pmc.WorkingSetSize;

        std::cout << max_node_data << ", ";
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(build_now - build_before).count() << ", ";
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(now - before).count() << ", ";
        std::cout << count << ", ";
        std::cout << vmem_used / 1e6 << ", ";

        log << max_node_data << ", ";
        log << std::chrono::duration_cast<std::chrono::milliseconds>(build_now - build_before).count() << ", ";
        log << std::chrono::duration_cast<std::chrono::milliseconds>(now - before).count() << ", ";
        log << count << ", ";
        log << vmem_used / 1e6 << ", ";

        begin = std::chrono::system_clock::now();
    }
    end = std::chrono::system_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;
    log << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;

    log.close();
}
#endif
