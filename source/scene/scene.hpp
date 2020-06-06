#pragma once

#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

#include "../surface/surface.hpp"
#include "../ray/ray.hpp"
#include "../common/bounding-box.hpp"

class Scene
{
public:
    Scene(const nlohmann::json& j);

    Intersection intersect(const Ray& ray, bool align_normal = false, double min_distance = -1) const;

    void generateEmissives();

    BoundingBox boundingBox(bool recompute);

    glm::dvec3 skyColor(const Ray& ray) const;

    std::vector<std::shared_ptr<Surface::Base>> surfaces;
    std::vector<std::shared_ptr<Surface::Base>> emissives; // subset of surfaces

    double ior;

protected:
    std::unique_ptr<BoundingBox> bounding_box;
};