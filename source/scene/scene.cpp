#include "scene.hpp"

Intersection Scene::intersect(const Ray& ray, bool align_normal, double min_distance) const
{
    bool use_stop_condition = min_distance > 0;

    Intersection intersect;
    for (const auto& surface : surfaces)
    {
        Intersection t_intersect;
        if (surface->intersect(ray, t_intersect))
        {
            if (use_stop_condition && t_intersect.t < min_distance)
                return Intersection();

            if (t_intersect.t < intersect.t)
                intersect = t_intersect;
        }
    }
    if (align_normal && glm::dot(ray.direction, intersect.normal) > 0)
    {
        intersect.normal = -intersect.normal;
    }
    return intersect;
}

void Scene::generateEmissives()
{
    for (const auto& surface : surfaces)
    {
        if (glm::length(surface->material->emittance) >= C::EPSILON)
        {
            surface->material->emittance /= surface->area(); // flux to radiosity
            emissives.push_back(surface);
        }
    }
}

BoundingBox Scene::boundingBox(bool recompute)
{
    if (surfaces.empty()) return BoundingBox();

    if (recompute || !bounding_box)
    {
        BoundingBox bb = surfaces.front()->boundingBox();
        for (const auto& surface : surfaces)
        {
            auto s_bb = surface->boundingBox();
            for (uint8_t c = 0; c < 3; c++)
            {
                bb.min[c] = std::min(bb.min[c], s_bb.min[c]);
                bb.max[c] = std::max(bb.max[c], s_bb.max[c]);
            }
        }
        bounding_box = std::make_unique<BoundingBox>(bb);
    }

    return *bounding_box;
}

glm::dvec3 Scene::skyColor(const Ray& ray) const
{
    double f = (glm::dot(glm::dvec3(0.0, 1.0, 0.0), ray.direction) * 0.7 + 1.0) / 2.0;
    if (f < 0.5)
    {
        return glm::dvec3(0.5); // ground color
    }
    return glm::dvec3(1.0 - f, 0.5, f);
}