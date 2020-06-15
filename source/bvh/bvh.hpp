#pragma once

#include "../surface/surface.hpp"
#include "../octree/octree.hpp"
#include "../ray/intersection.hpp"

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
    struct NodeIntersection
    {
        NodeIntersection(std::shared_ptr<BVHNode> node, double t) : t(t), node(node) { }

        bool operator< (const NodeIntersection& i) const
        {
            return i.t < t;
        }

        double t;
        std::shared_ptr<BVHNode> node;
    };
};

enum HiearchyMethod
{
    OCTREE,
    BINARY_SAH,
    OCTONARY_SAH
};

class BVH
{
public:
    BVH(const BoundingBox &BB, 
        const std::vector<std::shared_ptr<Surface::Base>> &surfaces, 
        HiearchyMethod miearchy_method);

    Intersection intersect(const Ray& ray);

    std::shared_ptr<BVHNode> root;

    const size_t leaf_surfaces = 8;
    std::map<size_t, size_t> branching;

    double branchingFactor();

private:
    void recursiveBuildFromOctree(const Octree<SurfacePoint> &octree_node, std::shared_ptr<BVHNode> bvh_node);
    void recursiveBuildBinarySAH(std::shared_ptr<BVHNode> bvh_node);
    void recursiveBuildOctonarySAH(std::shared_ptr<BVHNode> bvh_node);
};
