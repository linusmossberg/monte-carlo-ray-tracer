#include "bounding-box.hpp"

#include <algorithm>
#include <glm/gtx/component_wise.hpp>

bool BoundingBox::intersect(const Ray &ray, double &t) const
{
    t = 0.0;
    double t_max = std::numeric_limits<double>::max();

    for (int i = 0; i < 3; i++)
    {
        double inv_d = 1.0 / ray.direction[i];

        double t0 = (min[i] - ray.start[i]) * inv_d;
        double t1 = (max[i] - ray.start[i]) * inv_d;

        if (inv_d < 0.0)
        {
            std::swap(t0, t1);
        }

        if (t0 > t) t = t0;
        if (t1 < t_max) t_max = t1;

        if (t_max < t)
        {
            return false;
        }
    }

    return true;
}

bool BoundingBox::contains(const glm::dvec3 &p) const
{
    return p.x >= min.x && p.y >= min.y && p.z >= min.z && 
           p.x <= max.x && p.y <= max.y && p.z <= max.z;
}