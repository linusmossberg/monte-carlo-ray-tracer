#pragma once

#include <vector>
#include <thread>
#include <memory>

#include "../surface/Surface.hpp"
#include "../ray/Ray.hpp"
#include "../random/Random.hpp"
#include "../common/Intersection.hpp"
#include "../common/BoundingBox.hpp"

class Scene
{
public:
    Scene(std::vector<std::shared_ptr<Surface::Base>> surfaces, size_t sqrtspp, const std::string& savename, int threads, double ior);

    Intersection intersect(const Ray& ray, bool align_normal = false, double min_distance = -1);

    glm::dvec3 sampleDirect(const Intersection& intersection);

    void generateEmissives();

    BoundingBox boundingBox(bool recompute);

    glm::dvec3 skyColor(const Ray& ray) const;

    std::vector<std::shared_ptr<Surface::Base>> surfaces;
    std::vector<std::shared_ptr<Surface::Base>> emissives; // subset of surfaces

    size_t sqrtspp;
    size_t num_threads;
    std::string savename;
    double ior;

private:
    std::unique_ptr<BoundingBox> bounding_box;
};