#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

class OctreeData
{
public:
    virtual glm::vec3 pos() const = 0;
};

class Octree
{
public:
    Octree(const glm::vec3& origin, const glm::vec3& half_size, uint16_t max_points)
        : origin(origin), half_size(half_size), children(8), max_points(max_points) { }

    uint8_t getOctantContainingPoint(const glm::vec3& point) const 
    {
        uint8_t octant = 0;
        if (point.x >= origin.x) octant |= 0b100;
        if (point.y >= origin.y) octant |= 0b010;
        if (point.z >= origin.z) octant |= 0b001;
        return octant;
    }

    bool leaf() const
    {
        return !children[0];
    }

    void insert(std::unique_ptr<OctreeData> point)
    {
        if (leaf())
        {
            if (data.size() < max_points)
            {
                data.push_back(std::move(point));
            }
            else
            {
                std::vector<std::unique_ptr<OctreeData>> new_data = std::move(data);

                for (size_t i = 0; i < children.size(); i++)
                {
                    glm::vec3 new_origin = origin;
                    new_origin.x += half_size.x * (i & 0b100 ? 0.5f : -0.5f);
                    new_origin.y += half_size.y * (i & 0b010 ? 0.5f : -0.5f);
                    new_origin.z += half_size.z * (i & 0b001 ? 0.5f : -0.5f);
                    children[i] = std::make_unique<Octree>(new_origin, half_size * 0.5f, max_points);
                }

                children[getOctantContainingPoint(point->pos())]->insert(std::move(point));

                for (int i = 0; i < new_data.size(); i++)
                {
                    children[getOctantContainingPoint(new_data[i]->pos())]->insert(std::move(new_data[i]));
                }
            }
        }
        else
        {
            children[getOctantContainingPoint(point->pos())]->insert(std::move(point));
        }
    }

    void getPointsInBox(const glm::vec3& bmin, const glm::vec3& bmax, std::vector<OctreeData*>& result)
    {
        if (leaf())
        {
            for (const auto& d : data)
            {
                const auto p = d->pos();
                if (p.x > bmax.x || p.y > bmax.y || p.z > bmax.z) continue;
                if (p.x < bmin.x || p.y < bmin.y || p.z < bmin.z) continue;
                result.push_back(d.get());
            }
        }
        else
        {
            for (const auto &child : children)
            {
                glm::vec3 cmax = child->origin + child->half_size;
                glm::vec3 cmin = child->origin - child->half_size;

                if (cmax.x < bmin.x || cmax.y < bmin.y || cmax.z < bmin.z) continue;
                if (cmin.x > bmax.x || cmin.y > bmax.y || cmin.z > bmax.z) continue;

                child->getPointsInBox(bmin, bmax, result);
            }
        }
    }

    void getPointsInRadius(const glm::vec3& point, float radius, std::vector<OctreeData*>& result)
    {
        getPointsInBox(point - glm::vec3(radius), point + glm::vec3(radius), result);

        std::vector<OctreeData*>::iterator i;
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
    }

private:
    glm::vec3 origin;
    glm::vec3 half_size;

    uint16_t max_points;
    std::vector<std::unique_ptr<OctreeData>> data;

    std::vector<std::unique_ptr<Octree>> children;
};