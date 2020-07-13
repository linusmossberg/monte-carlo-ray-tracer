#include <iostream>
#include <fstream>
#include <string>
#include <chrono>

#ifdef _WIN32
    #include "windows.h"
    #include "psapi.h"
#endif

#include "../octree/linear-octree.cpp"
#include "../integrator/photon-mapper/photon-mapper.hpp"
#include "../random/random.hpp"
#include "../surface/surface.hpp"

void PhotonMapper::test(std::ostream& log, size_t num_iterations) const
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
    if (!scene.surfaces.empty())
    {
        for (int i = 0; i < num_iterations; i++)
        {
            const auto& surface = scene.surfaces[Random::get<size_t>(0, scene.surfaces.size() - 1)]; // random surface
            glm::dvec3 point = surface->operator()(Random::unit(), Random::unit()); // random point on surface

            auto caustic_photons = linear_caustic_map.knnSearch(point, k_nearest_photons, max_caustic_radius);
            auto direct_photons = linear_direct_map.knnSearch(point, k_nearest_photons, max_radius);
            auto indirect_photons = linear_indirect_map.knnSearch(point, k_nearest_photons, max_radius);
            auto shadow_photons = linear_shadow_map.knnSearch(point, k_nearest_photons, max_radius);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    log << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000.0 << std::endl;
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000.0 << std::endl;
#else
    std::cout << "Photon map testing is only supported on windows at the moment." << std::endl;
#endif
}

