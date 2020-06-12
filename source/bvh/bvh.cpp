#include "bvh.hpp"

#include <queue>

#include "../octree/octree.cpp"

BVH::BVH(const BoundingBox &BB, const std::vector<std::shared_ptr<Surface::Base>> &surfaces)
{
    double max_half_dim = glm::compMax(BB.max - BB.min) / 2.0;
    glm::dvec3 center = (BB.max + BB.min) / 2.0;

    Octree<SurfacePoint> hierarchy(BoundingBox(center - max_half_dim, center + max_half_dim), 10);

    for (const auto &s : surfaces)
    {
        hierarchy.insert(SurfacePoint(s));
    }

    root = std::make_shared<BVHNode>();
    root->BB = BB;

    recursiveBuild(hierarchy, root);
}

void BVH::recursiveBuild(const Octree<SurfacePoint> &node, std::shared_ptr<BVHNode> bvh)
{
    glm::dvec3 max_point(std::numeric_limits<double>::max());
    glm::dvec3 min_point(std::numeric_limits<double>::lowest());
    BoundingBox BB(max_point, min_point);

    if (node.leaf() && bvh)
    {
        bvh->surfaces.resize(node.data_vec.size());
        for (size_t i = 0; i < bvh->surfaces.size(); i++)
        {
            bvh->surfaces[i] = node.data_vec[i].surface;

            const auto &s_BB = bvh->surfaces[i]->boundingBox();
            for (int i = 0; i < 3; i++)
            {
                if (BB.max[i] < s_BB.max[i]) BB.max[i] = s_BB.max[i];
                if (BB.min[i] > s_BB.min[i]) BB.min[i] = s_BB.min[i];
            }
        }
    }
    else
    {
        for (size_t i = 0; i < node.octants.size(); i++)
        {
            if (!(node.octants[i]->leaf() && node.octants[i]->data_vec.empty()))
            {
                std::shared_ptr<BVHNode> child = std::make_shared<BVHNode>();
                bvh->children.push_back(child);
                recursiveBuild(*node.octants[i], child);

                const auto &c_BB = child->BB;
                for (int i = 0; i < 3; i++)
                {
                    if (BB.max[i] < c_BB.max[i]) BB.max[i] = c_BB.max[i];
                    if (BB.min[i] > c_BB.min[i]) BB.min[i] = c_BB.min[i];
                }
            }
        }
    }
    bvh->BB = BB;
}

Intersection BVH::intersect(const Ray& ray, bool align_normal)
{
    Intersection intersect;
    double t;
    if (root->BB.intersect(ray, t))
    {
        std::priority_queue<BVHNode::Intersection> intersected;
        intersected.emplace(root, t);

        while (!intersected.empty() && intersected.top().t < intersect.t)
        {
            auto current_node = intersected.top().node;
            intersected.pop();

            if (current_node->leaf())
            {
                for (const auto& surface : current_node->surfaces)
                {
                    Intersection t_intersect;
                    if (surface->intersect(ray, t_intersect))
                    {
                        if (t_intersect.t < intersect.t)
                        {
                            intersect = t_intersect;
                        }
                    }
                }
            }
            else
            {
                for (const auto &child : current_node->children)
                {
                    if (child->BB.intersect(ray, t))
                    {
                        intersected.emplace(child, t);
                    }
                }
            }
        }
    }

    if (intersect && align_normal && glm::dot(ray.direction, intersect.normal) > 0.0)
    {
        intersect.normal = -intersect.normal;
    }

    return intersect;
}