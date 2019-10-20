#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Constants.hpp"

struct OctreeData
{
    virtual glm::dvec3 pos() const = 0;
};

template <class Data>
class Octree
{
static_assert(std::is_base_of<OctreeData, Data>::value, "Octree Data type must derive from OctreeData.");
public:
    Octree(const glm::dvec3& origin, const glm::dvec3& half_size, uint16_t max_node_data)
        : origin(origin), half_size(half_size), octants(0), max_node_data(max_node_data) { }

    void insert(const Data& data)
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

                // allocating octant pointers here reduces memory usage drastically.  
                // Otherwise each leaf would have 8 unused pointers (8*64 bit = 64 bytes) in the final tree.
                octants.resize(8);
                for (uint8_t i = 0; i < octants.size(); i++)
                {
                    glm::dvec3 new_origin = origin;
                    for (uint8_t c = 0; c < 3; c++)
                    {
                        new_origin[c] += half_size[c] * (i & (0b100 >> c) ? 0.5 : -0.5);
                    }
                    octants[i] = std::make_shared<Octree>(new_origin, half_size * 0.5, max_node_data);
                }

                for (const auto &d : temp_data)
                {
                    insertInOctant(d);
                }
            }
        }
        insertInOctant(data);
    }

    std::vector<Data> boxSearch(const glm::dvec3& min, const glm::dvec3& max) const
    {
        std::vector<Data> result;
        boxRecursiveSearch(min, max, result);

        return result;
    }

    std::vector<Data> radiusSearch(const glm::dvec3& point, double radius) const
    {
        std::vector<Data> result;
        radiusRecursiveSearch(point, pow2(radius), result);
        return result;
    }

private:
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
    void insertInOctant(const Data& data)
    {
        uint8_t octant = 0;
        for (uint8_t c = 0; c < 3; c++)
        {
            if (data.pos()[c] >= origin[c]) octant |= (0b100 >> c);
        }
        octants[octant]->insert(data);
    }

    bool leaf() const
    {
        return octants.empty();
    }

    void boxRecursiveSearch(const glm::dvec3& min, const glm::dvec3& max, std::vector<Data>& result) const
    {
        if (leaf())
        {
            for (const auto& d : data_vec)
            {
                const auto p = d.pos();
                if (p.x < max.x && p.y < max.y && p.z < max.z &&
                    p.x > min.x && p.y > min.y && p.z > min.z)
                {
                    result.push_back(d);
                }
            }
        }
        else
        {
            for (const auto& octant : octants)
            {
                glm::dvec3 min_c = octant->origin - octant->half_size;
                glm::dvec3 max_c = octant->origin + octant->half_size;

                if (max_c.x > min.x && max_c.y > min.y && max_c.z > min.z &&
                    min_c.x < max.x && min_c.y < max.y && min_c.z < max.z)
                {
                    octant->boxRecursiveSearch(min, max, result);
                }
            }
        }
    }

    void radiusRecursiveSearch(const glm::dvec3& p, double radius2, std::vector<Data>& result) const
    {
        if (leaf())
        {
            for (const auto& data : data_vec)
            {
                if(glm::distance2(data.pos(), p) <= radius2)
                {
                    result.push_back(data);
                }
            }
        }
        else
        {
            for (const auto& octant : octants)
            {
                glm::dvec3 d(0.0);

                glm::dvec3 min = octant->origin - octant->half_size;
                glm::dvec3 max = octant->origin + octant->half_size;

                for (uint8_t c = 0; c < 3; c++)
                {
                    if (p[c] < min[c])
                        d[c] = pow2(p[c] - min[c]);
                    else if (p[c] > max[c])
                        d[c] = pow2(p[c] - max[c]);
                }

                if(d.x + d.y + d.z <= radius2)
                {
                    octant->radiusRecursiveSearch(p, radius2, result);
                }
            }
        }
    }

    glm::dvec3 origin;
    glm::dvec3 half_size;

    uint16_t max_node_data;

    std::vector<Data> data_vec;
    std::vector<std::shared_ptr<Octree>> octants;
};