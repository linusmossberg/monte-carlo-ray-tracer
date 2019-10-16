#pragma once

#include <vector>
#include <list>
#include <memory>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vector_relational.hpp>

class OctreeData
{
public:
    virtual glm::vec3 pos() const = 0;
};

class Octree
{
public:
    Octree(const glm::vec3& origin, const glm::vec3& half_size, uint16_t max_node_points)
        : origin(origin), half_size(half_size), children(8), max_node_points(max_node_points) { }

    void insert(const std::shared_ptr<OctreeData>& data)
    {
        if (leaf())
        {
            if (data_vec.size() < max_node_points)
            {
                data_vec.push_back(data); 
                return;
            }
            else
            {
                std::vector<std::shared_ptr<OctreeData>> temp_data = std::move(data_vec);

                for (uint8_t i = 0; i < children.size(); i++)
                {
                    glm::vec3 new_origin = origin;
                    for (uint8_t c = 0; c < 3; c++)
                    {
                        new_origin[c] += half_size[c] * (i & (0b100 >> c) ? 0.5f : -0.5f);
                    }
                    children[i] = std::make_unique<Octree>(new_origin, half_size * 0.5f, max_node_points);
                }

                for (const auto &d : temp_data)
                {
                    insertInChildOctant(d);
                }
            }
        }
        insertInChildOctant(data);
    }

    void dataInBox(const glm::vec3& min, const glm::vec3& max, std::vector<std::shared_ptr<OctreeData>>& result) const
    {
        if (leaf())
        {
            for (const auto &d : data_vec)
            {
                const auto p = d->pos();
                if (p.x < max.x && p.y < max.y && p.z < max.z &&
                    p.x > min.x && p.y > min.y && p.z > min.z)
                {
                    result.push_back(d);
                }
            }
        }
        else
        {
            for (const auto &child : children)
            {
                glm::vec3 min_c = child->origin - child->half_size;
                glm::vec3 max_c = child->origin + child->half_size;

                if (max_c.x > min.x && max_c.y > min.y && max_c.z > min.z &&
                    min_c.x < max.x && min_c.y < max.y && min_c.z < max.z)
                {
                    child->dataInBox(min, max, result);
                }
            }
        }
    }

    std::vector<std::shared_ptr<OctreeData>> dataInRadius(const glm::vec3& point, float radius) const
    {
        std::vector<std::shared_ptr<OctreeData>> result;
        dataInBox(point - glm::vec3(radius), point + glm::vec3(radius), result);

        std::vector<std::shared_ptr<OctreeData>>::iterator i;
        for (i = result.begin(); i != result.end(); )
        {
            if (glm::distance((*i)->pos(), point) > radius)
            {
                i = result.erase(i);
            }
            else
            {
                ++i;
            }
        }
        return result;
    }

private:
    void insertInChildOctant(const std::shared_ptr<OctreeData>& data)
    {
        uint8_t octant = 0;
        for (uint8_t c = 0; c < 3; c++)
        {
            if (data->pos()[c] >= origin[c]) octant |= (0b100 >> c);
        }
        children[octant]->insert(data);
    }

    bool leaf() const
    {
        return !children[0];
    }

    glm::vec3 origin;
    glm::vec3 half_size;

    uint16_t max_node_points;

    std::vector<std::shared_ptr<OctreeData>> data_vec;
    std::vector<std::unique_ptr<Octree>> children;
};