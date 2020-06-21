#pragma once

#include <vector>
#include <memory>
#include <queue>

#include <glm/vec3.hpp>

#include "../common/bounding-box.hpp"

struct OctreeData
{
    virtual glm::dvec3 pos() const = 0;
    virtual ~OctreeData() { }
};

template <class Data>
struct SearchResult
{
    SearchResult(const Data& data, double distance2) : data(data), distance2(distance2) { }
    Data data;
    double distance2;
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

    bool leaf() const
    {
        return octants.empty();
    }

    std::vector<SearchResult<Data>> radiusSearch(const glm::dvec3& point, double radius) const;
    std::vector<SearchResult<Data>> knnSearch(const glm::dvec3& p, size_t k, double max_distance);

    std::vector<Data> data_vec;
    BoundingBox BB;
    std::vector<std::unique_ptr<Octree>> octants;

private:
    void insertInOctant(const Data& data);

    void recursiveRadiusSearch(const glm::dvec3& p, double radius2, std::vector<SearchResult<Data>>& result) const;

    struct KNNode
    {
        KNNode(Octree* octant, double distance2) : octant(octant), distance2(distance2) { }
        KNNode(std::shared_ptr<Data> data, double distance2) : octant(nullptr), data(data), distance2(distance2)  { }

        bool operator< (const KNNode& e) const { return e.distance2 < distance2; };

        Octree* octant;
        std::shared_ptr<Data> data;
        double distance2;
    };

    size_t max_node_data;
};