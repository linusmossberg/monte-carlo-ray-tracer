#include "octree.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include "../common/constexpr-math.hpp"
#include "../common/constants.hpp"
#include "../common/util.hpp"

/**************
Octant:  x y z
     0:  0 0 0
     1:  0 0 1
     2:  0 1 0
     3:  0 1 1
     4:  1 0 0
     5:  1 0 1
     6:  1 1 0
     7:  1 1 1
***************/

template <class Data>
Octree<Data>::Octree(const glm::dvec3& origin, const glm::dvec3& half_size, size_t max_node_data)
    : BB(origin - half_size, origin + half_size), octants(0), max_node_data(max_node_data) { }

template <class Data>
Octree<Data>::Octree(const BoundingBox& bb, size_t max_node_data)
    : BB(bb), octants(0), max_node_data(max_node_data) { }

template <class Data>
Octree<Data>::Octree()
    : octants(0), max_node_data(190), BB() { }

template <class Data>
void Octree<Data>::insert(const Data& data)
{
    if (leaf())
    {
        if (data_vec.size() < max_node_data)
        {
            data_vec.push_back(data);
            return;
        }
        else
        {
            std::vector<Data> temp_data = std::move(data_vec);

            // Allocating octant pointers here reduces memory usage drastically.  
            // Otherwise each leaf would have 8 unused pointers (8*64 bit = 64 bytes) in the final tree.
            octants.resize(8);
            for (uint8_t i = 0; i < octants.size(); i++)
            {
                glm::dvec3 new_origin = BB.centroid();
                glm::dvec3 half_size = BB.dimensions() / 2.0;
                for (uint8_t c = 0; c < 3; c++)
                {
                    new_origin[c] += half_size[c] * (i & (0b100 >> c) ? 0.5 : -0.5);
                }
                octants[i] = std::make_unique<Octree>(new_origin, half_size * 0.5, max_node_data);
            }

            for (const auto& d : temp_data)
            {
                insertInOctant(d);
            }
        }
    }
    insertInOctant(data);
}

template <class Data>
std::vector<SearchResult<Data>> Octree<Data>::radiusSearch(const glm::dvec3& point, double radius) const
{
    std::vector<SearchResult<Data>> result;
    recursiveRadiusSearch(point, pow2(radius), result);
    return result;
}

template <class Data>
void Octree<Data>::insertInOctant(const Data& data)
{
    glm::dvec3 origin = BB.centroid();
    uint8_t octant = 0;
    for (uint8_t c = 0; c < 3; c++)
    {
        if (data.pos()[c] >= origin[c]) octant |= (0b100 >> c);
    }
    octants[octant]->insert(data);
}

// Squared distances/radius to avoid sqrt
template <class Data>
void Octree<Data>::recursiveRadiusSearch(const glm::dvec3& p, double radius2, std::vector<SearchResult<Data>>& result) const
{
    if (leaf())
    {
        for (const auto& data : data_vec)
        {
            double distance2 = glm::distance2(data.pos(), p);
            if (distance2 <= radius2)
            {
                result.emplace_back(data, distance2);
            }
        }
    }
    else
    {
        for (const auto& octant : octants)
        {
            // Keep searching in octants that intersects or is contained in the search sphere
            if (octant->BB.distance2(p) <= radius2)
            {
                octant->recursiveRadiusSearch(p, radius2, result);
            }
        }
    }
}

template <class Data>
std::vector<SearchResult<Data>> Octree<Data>::knnSearch(const glm::dvec3& p, size_t k, double radius_est)
{
    std::vector<SearchResult<Data>> result;
    result.reserve(k);

    double min_distance2 = -1.0;
    double max_distance2 = pow2(radius_est);
    double distance2 = BB.distance2(p);

    auto to_visit = reservedPriorityQueue<KNNode>(64);

    KNNode current(this, distance2);

    while (true)
    {
        if (current.octant)
        {
            if (current.octant->leaf())
            {
                for (const auto &data : current.octant->data_vec)
                {
                    double distance2 = glm::distance2(data.pos(), p);
                    if (distance2 <= max_distance2 && distance2 > min_distance2)
                    {
                        to_visit.emplace(std::make_shared<Data>(data), distance2);
                    }
                }
            }
            else
            {
                for (const auto &octant : current.octant->octants)
                {
                    double distance2 = octant->BB.distance2(p);
                    if (distance2 <= max_distance2 && octant->BB.max_distance2(p) > min_distance2)
                    {
                        to_visit.emplace(octant.get(), distance2);
                    }
                }
            }
        }
        else
        {
            result.emplace_back(*current.data, distance2);
            if (result.size() == k) return result;
        }

        if (to_visit.empty())
        {
            // Octree evidently contains less than k elements
            if (max_distance2 > BB.max_distance2(p)) return result;

            // Maximum search sphere doesn't contain k points. Increase radius and
            // traverse octree again, ignoring the already found closest points.
            max_distance2 *= 2.0;
            if (!result.empty()) min_distance2 = result.back().distance2;
            current = KNNode(this, BB.distance2(p));
        }
        else
        {
            current = to_visit.top();
            to_visit.pop();
        }

        if (to_visit.empty()) return result;

        current = to_visit.top();
        to_visit.pop();
    }
}
