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

    void emitPhoton(Ray ray, glm::dvec3 flux, size_t thread);

    virtual glm::dvec3 sampleRay(Ray ray);
    
    glm::dvec3 estimateGlobalRadiance(const Interaction& interaction); // All radiance except caustic
    glm::dvec3 estimateCausticRadiance(const Interaction& interaction);

private:
    LinearOctree<Photon> caustic_map;
    LinearOctree<Photon> global_map; // all photons except caustic photons

    // Temporary photon maps which are filled by each thread in the first pass. The Octree can't handle
    // concurrent inserts, so this has to be done if multi-threading is to be used in the first pass.
    std::vector<std::vector<Photon>> caustic_vecs;
    std::vector<std::vector<Photon>> global_vecs;

    double non_caustic_reject;

    bool direct_visualization;

    uint16_t max_node_data;
    size_t k_nearest_photons;
};
