#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/norm.hpp>

#include "../common/BoundingBox.hpp"
#include "../common/Util.hpp"
#include "../common/Constants.hpp"

struct OctreeData
{
    virtual glm::dvec3 pos() const = 0;
};

template <class Data>
class Octree
{
static_assert(std::is_base_of<OctreeData, Data>::value, "Octree Data type must derive from OctreeData.");

public:
    Octree(const glm::dvec3& origin, const glm::dvec3& half_size, uint16_t max_node_data);

    Octree(const BoundingBox& bb, uint16_t max_node_data);

    void insert(const Data& data);

    std::vector<Data> boxSearch(const glm::dvec3& min, const glm::dvec3& max) const;
    std::vector<Data> radiusSearch(const glm::dvec3& point, double radius) const;

private:
    void insertInOctant(const Data& data);

    bool leaf() const
    {
        return octants.empty();
    }

    void recursiveBoxSearch(const glm::dvec3& min, const glm::dvec3& max, std::vector<Data>& result) const;
    void recursiveRadiusSearch(const glm::dvec3& p, double radius2, std::vector<Data>& result) const;

    glm::dvec3 origin;
    glm::dvec3 half_size;

    uint16_t max_node_data;

    std::vector<Data> data_vec;
    std::vector<std::shared_ptr<Octree>> octants;
};