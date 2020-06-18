#pragma once

#include <vector>

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

#include "../integrator.hpp"

#include "../../octree/octree.hpp"
#include "photon.hpp"

class PhotonMapper : public Integrator
{
public:
    PhotonMapper(const nlohmann::json& j);

    void emitPhoton(const Ray& ray, const glm::dvec3& flux, size_t thread, size_t ray_depth = 0);

    void createShadowPhotons(const Ray& ray, size_t thread);

    virtual glm::dvec3 sampleRay(Ray ray, size_t ray_depth = 0);

    glm::dvec3 estimateRadiance(Octree<Photon>& map, const Intersection& intersect,
                                const glm::dvec3& direction, const CoordinateSystem& cs, double r) const;
    
    glm::dvec3 estimateCausticRadiance(const Intersection& intersect, const glm::dvec3& direction, const CoordinateSystem& cs);

    bool hasShadowPhoton(const Intersection& intersect) const
    {
        return !shadow_map.radiusSearch(intersect.position, radius).empty();
    }

    virtual glm::dvec3 sampleDirect(const Intersection& intersect, bool has_shadow_photons, bool use_direct_map) const;

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

    double radius;
    double caustic_radius;
    double non_caustic_reject;

    bool direct_visualization;

    uint16_t max_node_data;
    
    const size_t target_photons = 80;
};
