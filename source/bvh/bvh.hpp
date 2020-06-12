#pragma once

#include "../surface/surface.hpp"
#include "../octree/octree.hpp"

struct SurfacePoint : public OctreeData
{
    SurfacePoint(std::shared_ptr<Surface::Base> surface) : surface(surface) { }

    virtual glm::dvec3 pos() const
    {
        return surface->midPoint();
    }

    std::shared_ptr<Surface::Base> surface;
};

struct BVHNode
{
    BVHNode() { }

    bool leaf()
    {
        return children.empty();
    }

    BoundingBox BB;
    std::vector<std::shared_ptr<BVHNode>> children;
    std::vector<std::shared_ptr<Surface::Base>> surfaces;

    // Used for priority queue
    struct Intersection
    {
        Intersection(std::shared_ptr<BVHNode> node, double t) : t(t), node(node) { }

        bool operator< (const Intersection& i) const
        {
            return i.t < t;
        }

        double t;
        std::shared_ptr<BVHNode> node;
    };
};

struct BVH
{
    BVH(const BoundingBox &BB, const std::vector<std::shared_ptr<Surface::Base>> &surfaces);

    void recursiveBuild(const Octree<SurfacePoint> &node, std::shared_ptr<BVHNode> bvh);

    Intersection intersect(const Ray& ray, bool align_normal = true);

    std::shared_ptr<BVHNode> root;
};
