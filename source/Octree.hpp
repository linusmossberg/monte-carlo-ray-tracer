#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

struct OctreeData
{
    virtual glm::vec3 pos() const = 0;
};

template <class Data>
class Octree
{
static_assert(std::is_base_of<OctreeData, Data>::value, "Octree Data type must derive from OctreeData.");
public:
    Octree(const glm::vec3& origin, const glm::vec3& half_size, uint16_t max_node_data)
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
                    glm::vec3 new_origin = origin;
                    for (uint8_t c = 0; c < 3; c++)
                    {
                        new_origin[c] += half_size[c] * (i & (0b100 >> c) ? 0.5f : -0.5f);
                    }
                    octants[i] = std::make_unique<Octree>(new_origin, half_size * 0.5f, max_node_data);
                }

                for (const auto &d : temp_data)
                {
                    insertInOctant(d);
                }
            }
        }
        insertInOctant(data);
    }

    std::vector<Data> boxSearch(const glm::vec3& min, const glm::vec3& max) const
    {
        std::vector<Data> result;
        boxRecursiveSearch(min, max, result);

        return result;
    }

    std::vector<Data> radiusSearch(const glm::vec3& point, float radius) const
    {
        std::vector<Data> result;
        boxRecursiveSearch(point - glm::vec3(radius), point + glm::vec3(radius), result);

        for (auto i = result.begin(); i != result.end(); )
        {
            if (glm::distance(i->pos(), point) > radius)
                i = result.erase(i);
            else
                ++i;
        }
        return result;
    }

private:
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

    void boxRecursiveSearch(const glm::vec3& min, const glm::vec3& max, std::vector<Data>& result) const
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
                glm::vec3 min_c = octant->origin - octant->half_size;
                glm::vec3 max_c = octant->origin + octant->half_size;

                if (max_c.x > min.x && max_c.y > min.y && max_c.z > min.z &&
                    min_c.x < max.x && min_c.y < max.y && min_c.z < max.z)
                {
                    octant->boxRecursiveSearch(min, max, result);
                }
            }
        }
    }

    glm::vec3 origin;
    glm::vec3 half_size;

    uint16_t max_node_data;

    std::vector<Data> data_vec;
    std::vector<std::unique_ptr<Octree>> octants;
};