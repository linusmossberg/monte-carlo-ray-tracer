#pragma once

#include "octree.hpp"

struct Photon : public OctreeData
{
public:
    Photon(const glm::dvec3& flux, const glm::dvec3& position, const glm::dvec3& direction)
        : flux(flux), position(position), direction(direction) { }

    virtual glm::dvec3 pos() const
    {
        return position;
    }

    glm::dvec3 flux, position, direction;
};

// Separate shadow photon type to reduce memory usage
struct ShadowPhoton : public OctreeData
{
public:
    ShadowPhoton(const glm::dvec3& position)
        : position(position) { }

    virtual glm::dvec3 pos() const
    {
        return position;
    }

    glm::dvec3 position;
};
