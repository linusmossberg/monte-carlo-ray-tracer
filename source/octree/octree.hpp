#pragma once

#include <vector>
#include <memory>
#include <queue>

#include <glm/vec3.hpp>

#include "../common/bounding-box.hpp"
#include "../common/constexpr-math.hpp"

template <class Data>
struct alignas(nextPowerOfTwo(sizeof(Data) + sizeof(double))) SearchResult
{
    SearchResult(const Data& data, double distance2) : data(data), distance2(distance2) { }
    bool operator< (const SearchResult& rhs) const { return distance2 < rhs.distance2; };
    Data data;
    double distance2;
};

template <class Data>
class Octree
{
static_assert(
    std::is_member_function_pointer<decltype(&Data::pos)>::value, 
    "Octree Data must implement a 'glm::dvec3 pos()' member."
);
public:
    Octree(const glm::dvec3& origin, const glm::dvec3& half_size, size_t max_node_data);
    Octree(const BoundingBox& bb, size_t max_node_data);
    Octree();

    void insert(const Data& data);

    bool leaf() const
    {
        return octants.empty();
    }

    std::vector<Data> data_vec;
    BoundingBox BB;
    std::vector<std::unique_ptr<Octree>> octants;

private:
    void insertInOctant(const Data& data);

    size_t max_node_data;
};