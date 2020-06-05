#pragma once

#include <vector>
#include <thread>
#include <memory>

#include <glm/gtx/norm.hpp>

#include "../surface/surface.hpp"
#include "../ray/ray.hpp"
#include "../random/random.hpp"
#include "../common/intersection.hpp"
#include "../common/bounding-box.hpp"

class Scene
{
public:
    Scene(std::vector<std::shared_ptr<Surface::Base>> surfaces, int threads, double ior, bool naive)
        : surfaces(surfaces), ior(ior), naive(naive)
    {
        size_t max_threads = std::thread::hardware_concurrency();
        num_threads = (threads < 1 || threads > max_threads) ? max_threads : threads;
        std::cout << std::endl << "Threads used for rendering: " << num_threads << std::endl << std::endl;
        generateEmissives();
    }

    glm::dvec3 sampleRay(Ray ray, size_t ray_depth = 0);

    Intersection intersect(const Ray& ray, bool align_normal = false, double min_distance = -1);

    glm::dvec3 sampleDirect(const Intersection& intersection);

    void generateEmissives();

    BoundingBox boundingBox(bool recompute);

    glm::dvec3 skyColor(const Ray& ray) const;

    std::vector<std::shared_ptr<Surface::Base>> surfaces;
    std::vector<std::shared_ptr<Surface::Base>> emissives; // subset of surfaces

    size_t num_threads;
    double ior;

private:
    const size_t min_ray_depth = 3;
    const size_t max_ray_depth = 96; // prevent call stack overflow

    bool naive;

    std::unique_ptr<BoundingBox> bounding_box;
};