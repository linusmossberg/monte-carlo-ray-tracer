#pragma once

#include <nlohmann/json.hpp>

#include "../ray/intersection.hpp"
#include "../octree/octree.hpp"

namespace Surface { class Base; }

class BVH
{
    struct SurfaceCentroid : public OctreeData
    {
        SurfaceCentroid(std::shared_ptr<Surface::Base> surface);

        virtual const glm::dvec3& pos() const
        {
            return centroid;
        }

        glm::dvec3 centroid;
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
        uint32_t df_idx; // depth-first index in tree
    };

    /********************************************************************************
     Linear array node for N-ary trees. Currently 57B padded to 64B.

     For future reference, it's possible to get this to 29B and pad to 32B by:

     1. Using float instead of double vectors for the bounding box.
     2. Store the last descendant index (in depth first order) instead of next
        sibling index and set this to be union the surface offset (since leaves have
        no descendants and inner nodes have no surfaces).

     Traversal of a parents child nodes would then work like:

     if parent is not a leaf
         current_child = parent + 1
         while true
            [do stuff with current node]
            if current_child is a leaf
                if current_child is parents last descendant
                    break
                else
                    current_child = current_child + 1
            else
                if current_childs last descendant is parents last descendant
                    break
                else
                    current_child = current_childs last descendant + 1

    ********************************************************************************/
    struct alignas(64) LinearNode
    {
        BoundingBox BB;
        uint32_t start_surface;
        uint8_t num_surfaces;
        uint32_t next_sibling; // 0 if there is none

        // Used for priority queue
        struct NodeIntersection
        {
            NodeIntersection(uint32_t node, double t) : t(t), node(node) { }
            bool operator< (const NodeIntersection& i) const { return i.t < t; };
            double t;
            uint32_t node;
        };
    };

public:
    BVH(const BoundingBox &BB, 
        const std::vector<std::shared_ptr<Surface::Base>> &surfaces, 
        const nlohmann::json &j);

    Intersection intersect(const Ray& ray);

    const size_t leaf_surfaces = 8;
    const size_t max_leaf_surfaces = 0xFF;
    std::map<size_t, size_t> branching;

    int bins_per_axis = 16;

private:
    void recursiveBuildFromOctree(const Octree<SurfaceCentroid> &octree_node, std::shared_ptr<BuildNode> bvh_node);
    void recursiveBuildBinarySAH(std::shared_ptr<BuildNode> bvh_node);
    void recursiveBuildQuaternarySAH(std::shared_ptr<BuildNode> bvh_node);
    void compact(std::shared_ptr<BuildNode> bvh_node, uint32_t next_sibling, uint32_t &surface_idx);

    void arbitrarySplit(std::shared_ptr<BuildNode> bvh_node, size_t N);

    // Nodes stored in depth-first order
    std::vector<LinearNode> linear_tree;

    std::vector<std::shared_ptr<Surface::Base>> ordered_surfaces;

    // Depth first index used during construction
    uint32_t df_idx;
};