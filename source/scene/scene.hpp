#pragma once

#include <vector>
#include <memory>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "../surface/surface.hpp"
#include "../ray/ray.hpp"
#include "../common/bounding-box.hpp"

#include "../bvh/bvh.hpp"

class Scene
{
public:
    Scene(const nlohmann::json& j);

    Intersection intersect(const Ray& ray, bool shadow_ray = false) const;

    void generateEmissives();

    glm::dvec3 skyColor(const Ray& ray) const;

    std::vector<std::shared_ptr<Surface::Base>> surfaces;
    std::vector<std::shared_ptr<Surface::Base>> emissives; // subset of surfaces

    BoundingBox BB() const
    {
        return BB_;
    }

    std::unique_ptr<BVH> bvh;

    double ior;

private:
    BoundingBox BB_;

    void computeBoundingBox();

    void parseOBJ(const std::filesystem::path &path,
                  std::vector<glm::dvec3> &vertices,
                  std::vector<glm::dvec3> &normals,
                  std::vector<std::vector<size_t>> &triangles_v,
                  std::vector<std::vector<size_t>> &triangles_vt,
                  std::vector<std::vector<size_t>> &triangles_vn) const;
};