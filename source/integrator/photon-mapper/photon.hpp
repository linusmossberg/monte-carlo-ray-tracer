#pragma once

#include <glm/glm.hpp>

struct alignas(32) Photon
{
    Photon(const glm::dvec3& flux, const glm::dvec3& position, const glm::dvec3& direction)
        : flux_(flux), position_(position)
    {
        theta = (float)std::atan2(glm::length(glm::dvec2(direction)), direction.z);
        phi = (float)std::atan2(direction.y, direction.x);
    }

    glm::dvec3 pos() const
    {
        return position_;
    }

    glm::dvec3 dir() const
    {
        double sin_theta = std::sin(theta);
        return glm::dvec3(
            sin_theta * std::cos(phi),
            sin_theta * std::sin(phi),
            std::cos(theta)
        );
    }

    glm::dvec3 flux() const
    {
        return flux_;
    }

private:
    // Single-precision and polar angles to reduce size to 32 bytes.
    glm::vec3 flux_, position_;
    float phi, theta;
};
