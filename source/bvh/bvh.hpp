#pragma once

#include <nlohmann/json.hpp>

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

struct BuildNode
{
    BuildNode() { }

    bool leaf()
    {
        return children.empty();
    }

    BoundingBox BB;
    std::vector<std::shared_ptr<BuildNode>> children;
    std::vector<std::shared_ptr<Surface::Base>> surfaces;
    int32_t df_idx; // depth-first index in tree
};

struct LinearNode
{
    BoundingBox BB;
    std::vector<std::shared_ptr<Surface::Base>> surfaces;
    int32_t right_sibling; // -1 if there is none
    bool leaf;

    // Used for priority queue
    struct NodeIntersection
    {
        NodeIntersection(uint32_t node, double t) : t(t), node(node) { }
        bool operator< (const NodeIntersection& i) const { return i.t < t; };
        double t;
        uint32_t node;
    };
};

class BVH
{
public:
    BVH(const BoundingBox &BB, 
        const std::vector<std::shared_ptr<Surface::Base>> &surfaces, 
        const nlohmann::json &j);

    Intersection intersect(const Ray& ray);

    const size_t leaf_surfaces = 8;
    std::map<size_t, size_t> branching;

    int bins_per_axis = 16;

private:
    void recursiveBuildFromOctree(const Octree<SurfacePoint> &octree_node, std::shared_ptr<BuildNode> bvh_node);
    void recursiveBuildBinarySAH(std::shared_ptr<BuildNode> bvh_node);
    void recursiveBuildQuaternarySAH(std::shared_ptr<BuildNode> bvh_node);
    void compact(std::shared_ptr<BuildNode> bvh_node, int32_t idx);

    // Nodes stored in depth-first order
    std::vector<LinearNode> linear_tree;

    // Depth first index used during construction
    uint32_t df_idx;
};
