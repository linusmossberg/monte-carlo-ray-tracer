#pragma once

#include "octree.hpp"

template <class Data>
class LinearOctree
{
public:
    LinearOctree() { }

    // This destroys the input octree for memory reasons.
    LinearOctree(Octree<Data> &octree_root);

    std::vector<SearchResult<Data>> knnSearch(const glm::dvec3& p, size_t k, double max_distance) const;

    struct alignas(64) LinearOctant
    {
        BoundingBox BB;
        uint64_t start_data;
        uint16_t num_data;
        int32_t next_sibling;
        uint8_t leaf;
    };

    std::vector<LinearOctant> linear_tree;
    std::vector<Data> ordered_data;

private:
    void compact(Octree<Data> &node, int32_t &df_idx, uint64_t &data_idx, bool last = false);

    void octreeSize(const Octree<Data> &octree_root, size_t &size, size_t &data_size) const;

    struct KNNode
    {
        KNNode(int32_t octant, double distance2) : octant(octant), distance2(distance2) { }
        KNNode(uint64_t data, double distance2) : octant(-1), data(data), distance2(distance2) { }

        bool operator< (const KNNode& e) const { return e.distance2 < distance2; };

        int32_t octant;
        uint64_t data;
        double distance2;
    };
};
