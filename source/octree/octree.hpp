#pragma once

#include <vector>
#include <memory>
#include <queue>

#include <glm/vec3.hpp>

#include "../common/bounding-box.hpp"

struct OctreeData
{
    virtual const glm::dvec3& pos() const = 0;
    double distance2 = std::numeric_limits<double>::max();
    bool operator< (const OctreeData& d) const { return distance2 < d.distance2; };
};

template <class Data>
class Octree
{
static_assert(std::is_base_of<OctreeData, Data>::value, "Octree Data type must derive from OctreeData.");

public:
    Octree(const glm::dvec3& origin, const glm::dvec3& half_size, size_t max_node_data);

    Octree(const BoundingBox& bb, size_t max_node_data);

    Octree();

    void insert(const Data& data);

    std::vector<Data> boxSearch(const glm::dvec3& min, const glm::dvec3& max) const;
    std::vector<Data> radiusSearch(const glm::dvec3& point, double radius) const;

    bool leaf() const
    {
        return octants.empty();
    }

    std::vector<Data> data_vec;
    std::vector<std::unique_ptr<Octree>> octants;

    std::vector<Data> knnSearch(const glm::dvec3& p, size_t k, double max_distance);

private:
    void insertInOctant(const Data& data);

    void recursiveBoxSearch(const glm::dvec3& min, const glm::dvec3& max, std::vector<Data>& result) const;
    void recursiveRadiusSearch(const glm::dvec3& p, double radius2, std::vector<Data>& result) const;

    struct KNNode
    {
        KNNode(Octree* octant, double distance2) : octant(octant), distance2(distance2) { }
        KNNode(std::shared_ptr<Data> data, double distance2) : octant(nullptr), data(data), distance2(distance2)  { }

        bool operator< (const KNNode& e) const { return e.distance2 < distance2; };

        Octree* octant;
        std::shared_ptr<Data> data;
        double distance2;
    };

    BoundingBox BB;

    size_t max_node_data;
};