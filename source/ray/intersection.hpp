#pragma once

#include <memory>

#include <glm/vec2.hpp>

namespace Surface { class Base; }

struct Intersection
{
    Intersection() { }
    Intersection(double t) : t(t) { }
    std::shared_ptr<Surface::Base> surface;
    double t = (std::numeric_limits<double>::max)();

    glm::dvec2 uv;
    bool interpolate = false;

    explicit operator bool() const
    {
        return surface.operator bool();
    }
};