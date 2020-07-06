#pragma once

#include <vector>

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

#include "photon.hpp"
#include "../integrator.hpp"
#include "../../octree/linear-octree.hpp"

class PhotonMapper : public Integrator
{
public:
    PhotonMapper(const nlohmann::json& j);

    void emitPhoton(const Ray& ray, const glm::dvec3& flux, size_t thread, size_t ray_depth = 0);

    void createShadowPhotons(const Ray& ray, size_t thread, size_t depth = 0);

    virtual glm::dvec3 sampleRay(Ray ray, size_t ray_depth = 0);
    
    glm::dvec3 estimateRadiance(const Interaction& interaction, const std::vector<SearchResult<Photon>> &photons);
    glm::dvec3 estimateCausticRadiance(const Interaction& interaction);

    bool hasShadowPhotons(const Interaction& interaction) const;

    // Implemented in Tests.cpp
    void test(std::ostream& log, size_t num_iterations) const;

private:
    // direct, indirect and shadow maps are commonly combined into a global photon map
    LinearOctree<Photon> linear_caustic_map;
    LinearOctree<Photon> linear_direct_map;
    LinearOctree<Photon> linear_indirect_map;
    LinearOctree<ShadowPhoton> linear_shadow_map;

    // Temporary photon maps which are filled by each thread in the first pass. The Octree can't handle
    // concurrent inserts, so this has to be done if multi-threading is to be used in the first pass.
    std::vector<std::vector<Photon>> caustic_vecs;
    std::vector<std::vector<Photon>> direct_vecs;
    std::vector<std::vector<Photon>> indirect_vecs;
    std::vector<std::vector<ShadowPhoton>> shadow_vecs;

    double max_radius;
    double max_caustic_radius;
    double min_bounce_distance;
    double non_caustic_reject;

    bool direct_visualization;
    bool use_shadow_photons;

    uint16_t max_node_data;
    
    size_t k_nearest_photons;
};
