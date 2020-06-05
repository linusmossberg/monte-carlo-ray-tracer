#pragma once

#include <vector>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vector_relational.hpp>

#include "octree.hpp"
#include "octree.cpp" // Has to be included when methods are defined outside of templated class
#include "photon.hpp"

#include "../common/util.hpp"
#include "../scene/scene.hpp"
#include "../common/work-queue.hpp"
#include "../common/constants.hpp"
#include "../common/format.hpp"

class PhotonMap
{
public:
    PhotonMap(std::shared_ptr<Scene> s, size_t photon_emissions, uint16_t max_node_data, 
              double caustic_factor, double radius, double caustic_radius, bool direct_visualization, bool print = true);

    void emitPhoton(const Ray& ray, const glm::dvec3& flux, size_t thread, size_t ray_depth = 0);

    void createShadowPhotons(const Ray& ray, size_t thread);

    glm::dvec3 sampleRay(const Ray& ray, size_t ray_depth = 0);

    glm::dvec3 estimateRadiance(const Octree<Photon>& map, const Intersection& intersect,
                                const glm::dvec3& direction, const CoordinateSystem& cs, double r) const;
    
    glm::dvec3 estimateCausticRadiance(const Intersection& intersect, const glm::dvec3& direction, const CoordinateSystem& cs) const;

    bool hasShadowPhoton(const Intersection& intersect) const
    {
        return !shadow_map.radiusSearch(intersect.position, radius).empty();
    }

    glm::dvec3 sampleDirect(const Intersection& intersect, bool has_shadow_photons, bool use_direct_map) const;

    // Implemented in Tests.cpp
    void test(std::ostream& log, size_t num_iterations) const;

private:
    // direct, indirect and shadow maps are commonly combined into a global photon map
    Octree<Photon> caustic_map;
    Octree<Photon> direct_map;
    Octree<Photon> indirect_map;
    Octree<ShadowPhoton> shadow_map;

    // Temporary photon maps which are filled by each thread in the first pass. The Octree can't handle
    // concurrent inserts, so this has to be done if multi-threading is to be used in the first pass.
    std::vector<std::vector<Photon>> caustic_vecs;
    std::vector<std::vector<Photon>> direct_vecs;
    std::vector<std::vector<Photon>> indirect_vecs;
    std::vector<std::vector<ShadowPhoton>> shadow_vecs;

    std::shared_ptr<Scene> scene;

    double radius;
    double caustic_radius;
    double non_caustic_reject;

    bool direct_visualization;

    uint16_t max_node_data;

    const size_t min_ray_depth = 3;
    const size_t max_ray_depth = 96; // Prevent call stack overflow, unlikely to ever happen.
};
