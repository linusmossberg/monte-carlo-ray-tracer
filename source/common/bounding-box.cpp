#include "bounding-box.hpp"

bool BoundingBox::intersect(const Ray &ray, double &t) const
{
    double t_max = std::numeric_limits<double>::max();
    double t_min = -t_max;

    for (int i = 0; i < 3; i++)
    {
        double inv_d = 1.0 / ray.direction[i];

        double t0 = (min[i] - ray.start[i]) * inv_d;
        double t1 = (max[i] - ray.start[i]) * inv_d;

        if (inv_d < 0.0)
        {
            double temp = t0;
            t0 = t1;
            t1 = temp;
        }

        if (t0 > t_min) t_min = t0;
        if (t1 < t_max) t_max = t1;

        if (t_max < t_min)
        {
            return false;
        }
    }

    t = t_min;

    return true;
}

bool BoundingBox::contains(const glm::dvec3 &p) const
{
    return p.x >= min.x && p.y >= min.y && p.z >= min.z && 
           p.x <= max.x && p.y <= max.y && p.z <= max.z;
}