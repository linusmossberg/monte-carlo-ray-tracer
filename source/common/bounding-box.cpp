#include "bounding-box.hpp"

#include <algorithm>
#include <glm/gtx/component_wise.hpp>

#include "../ray/ray.hpp"

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

glm::dvec3 BoundingBox::dimensions() const
{
    return max - min;
}

glm::dvec3 BoundingBox::centroid() const
{
    return (max + min) / 2.0;
}

double BoundingBox::area() const
{
    if (!valid()) return 0.0;
    glm::dvec3 d = dimensions();
    return 2.0 * (d.x * d.y + d.x * d.z + d.y * d.z);
}

// Smallest possible squared distance to a point in the bounding box
double BoundingBox::distance2(const glm::dvec3 &p) const
{
    glm::dvec3 d = glm::max(glm::max(min - p, p - max), glm::dvec3(0.0));
    return glm::dot(d, d);
}

// Largest possible squared distance to a point in the bounding box
double BoundingBox::max_distance2(const glm::dvec3& p) const
{
    glm::dvec3 d = glm::max(max - p, p - min);
    return glm::dot(d, d);
}

void BoundingBox::merge(const BoundingBox &BB)
{
    for (int i = 0; i < 3; i++)
    {
        if (min[i] > BB.min[i]) min[i] = BB.min[i];
        if (max[i] < BB.max[i]) max[i] = BB.max[i];
    }
}

void BoundingBox::merge(const glm::dvec3 &p)
{
    for (int i = 0; i < 3; i++)
    {
        if (min[i] > p[i]) min[i] = p[i];
        if (max[i] < p[i]) max[i] = p[i];
    }
}

bool BoundingBox::valid() const
{
    for (int i = 0; i < 3; i++)
    {
        if (min[i] > max[i]) return false;
    }
    return true;
}