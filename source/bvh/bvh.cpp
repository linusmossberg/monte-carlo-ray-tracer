#include "bvh.hpp"

#include <queue>
#include <chrono>
#include <iostream>

#include "../octree/octree.cpp"
#include "../common/format.hpp"

BVH::BVH(const BoundingBox &BB, 
         const std::vector<std::shared_ptr<Surface::Base>> &surfaces, 
         HiearchyMethod hiearchy_method) : root(std::make_shared<BVHNode>())
{
    root->BB = BB;
    root->BB.computeProperties();

    auto begin = std::chrono::high_resolution_clock::now();
    std::cout << "\nNumber of primitives: " << Format::largeNumber(surfaces.size()) << std::endl;

    switch (hiearchy_method)
    {
        case HiearchyMethod::OCTREE:
        {
            std::cout << "\nBuilding BVH from octree.\n\n";

            double half_max = glm::compMax(root->BB.dimensions()) / 2.0;
            BoundingBox cube_BB(root->BB.centroid - half_max, root->BB.centroid + half_max);

            Octree<SurfacePoint> hierarchy(cube_BB, leaf_surfaces);

            for (const auto &s : surfaces)
            {
                hierarchy.insert(SurfacePoint(s));
            }

            recursiveBuildFromOctree(hierarchy, root);
            break;
        }
        case HiearchyMethod::BINARY_SAH:
        {
            std::cout << "\nBuilding binary BVH using SAH.\n\n";
            root->surfaces = surfaces;
            recursiveBuildBinarySAH(root);
            break;
        }
        case HiearchyMethod::OCTONARY_SAH:
        {
            std::cout << "\nBuilding octonary BVH using SAH.\n\n";
            root->surfaces = surfaces;
            recursiveBuildOctonarySAH(root);
            break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    size_t msec_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "BVH constructed in " + Format::timeDuration(msec_duration)
              << ". Branching factor of tree: " << branchingFactor() << std::endl << std::endl;
}

Intersection BVH::intersect(const Ray& ray)
{
    Intersection intersect;
    double t;
    if (root->BB.intersect(ray, t))
    {
        std::priority_queue<BVHNode::NodeIntersection> to_visit;
        to_visit.emplace(root, t);
        std::shared_ptr<BVHNode> current_node;

        while (!to_visit.empty() && to_visit.top().t < intersect.t)
        {
            current_node = to_visit.top().node;
            to_visit.pop();

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
                    if (child->BB.intersect(ray, t) && t < intersect.t)
                    {
                        to_visit.emplace(child, t);
                    }
                }
            }
        }
    }
    return intersect;
}

void BVH::recursiveBuildFromOctree(const Octree<SurfacePoint> &octree_node, std::shared_ptr<BVHNode> bvh_node)
{
    BoundingBox BB;

    if (octree_node.leaf())
    {
        bvh_node->surfaces.resize(octree_node.data_vec.size());
        for (size_t i = 0; i < bvh_node->surfaces.size(); i++)
        {
            bvh_node->surfaces[i] = octree_node.data_vec[i].surface;
            BB.merge(bvh_node->surfaces[i]->BB);
        }
    }
    else
    {
        size_t num_children = 0;
        for (size_t i = 0; i < octree_node.octants.size(); i++)
        {
            if (!(octree_node.octants[i]->leaf() && octree_node.octants[i]->data_vec.empty()))
            {
                std::shared_ptr<BVHNode> child = std::make_shared<BVHNode>();
                bvh_node->children.push_back(child);
                recursiveBuildFromOctree(*octree_node.octants[i], child);
                BB.merge(child->BB);
                num_children++;
            }
        }
        branching[num_children]++;
    }
    bvh_node->BB = BB;
}

void BVH::recursiveBuildBinarySAH(std::shared_ptr<BVHNode> bvh_node)
{
    int num_bins = 16;

    auto &S = bvh_node->surfaces;

    if (S.size() <= leaf_surfaces)
    {
        return;
    }

    BoundingBox centroid_extent;
    for (const auto &s : S)
    {
        centroid_extent.merge(s->midPoint());
    }
    glm::dvec3 extent_dims = centroid_extent.dimensions();

    uint8_t split_axis;
    if (extent_dims.x > extent_dims.y)
    {
        split_axis = extent_dims.x > extent_dims.z ? 0 : 2;
    }
    else
    {
        split_axis = extent_dims.y > extent_dims.z ? 1 : 2;
    }

    if (extent_dims[split_axis] <= 0.0)
    {
        return;
    }

    auto getIdx = [&](const glm::dvec3 &centroid)
    {
        double f = (centroid[split_axis] - centroid_extent.min[split_axis]) / extent_dims[split_axis];
        int idx = glm::floor(f * num_bins);
        idx = glm::min(idx, num_bins - 1);
        return idx;
    };

    std::vector<std::pair<size_t, BoundingBox>> bins(num_bins, { 0, BoundingBox() });
    for (const auto &s : S)
    {
        int idx = getIdx(s->midPoint());
        bins[idx].first++;
        bins[idx].second.merge(s->BB);
    }

    std::shared_ptr<BVHNode> A = std::make_shared<BVHNode>();
    std::shared_ptr<BVHNode> B = std::make_shared<BVHNode>();

    double min_cost = std::numeric_limits<double>::max();
    size_t split_bin = 0;

    for (size_t i = 0; i < bins.size() - 1; i++)
    {
        size_t A_bins = i + 1;
        size_t B_bins = bins.size() - A_bins;

        size_t A_count = 0;
        BoundingBox A_BB;
        for (size_t j = 0; j < A_bins; j++)
        {
            A_count += bins[j].first;
            A_BB.merge(bins[j].second);
        }

        size_t B_count = 0;
        BoundingBox B_BB;
        for (size_t j = A_bins; j < bins.size(); j++)
        {
            B_count += bins[j].first;
            B_BB.merge(bins[j].second);
        }

        double cost = 1.0 + (A_count * A_BB.area() + B_count * B_BB.area()) / bvh_node->BB.area();

        if (cost < min_cost)
        {
            split_bin = i;
            min_cost = cost;
        }
    }

    if (min_cost > S.size())
    {
        return;
    }

    for (const auto &s : S)
    {
        int idx = getIdx(s->midPoint());
        if (idx <= split_bin)
        {
            A->surfaces.push_back(s);
            A->BB.merge(s->BB);
        }
        else
        {
            B->surfaces.push_back(s);
            B->BB.merge(s->BB);
        }
    }

    A->BB.computeProperties();
    B->BB.computeProperties();

    S.clear();

   
    size_t num_children = 0;

    if (!A->surfaces.empty())
    {
        bvh_node->children.push_back(A);
        recursiveBuildBinarySAH(A);
        num_children++;
    }
    if (!B->surfaces.empty())
    {
        bvh_node->children.push_back(B);
        recursiveBuildBinarySAH(B);
        num_children++;
    }

    branching[num_children]++;
}

void BVH::recursiveBuildOctonarySAH(std::shared_ptr<BVHNode> bvh_node)
{
    glm::ivec3 num_bins(8);

    auto &S = bvh_node->surfaces;

    if (S.size() <= leaf_surfaces)
    {
        return;
    }

    BoundingBox centroid_extent;
    for (const auto &s : S)
    {
        centroid_extent.merge(s->midPoint());
    }
    glm::dvec3 extent_dims = centroid_extent.dimensions();

    if (glm::compMax(extent_dims) <= 0.0)
    {
        return;
    }

    glm::dvec3 bin_width = extent_dims / glm::dvec3(num_bins);
    centroid_extent.max += bin_width;
    centroid_extent.min -= bin_width;
    extent_dims += 2.0 * bin_width;
    num_bins += 2;

    glm::dvec3 inv_extent_dims(0.0);
    for (uint8_t c = 0; c < 3; c++)
    {
        if (extent_dims[c] <= 0.0)
        {
            num_bins[c] = 2;
        }
        else
        {
            inv_extent_dims[c] = 1.0 / extent_dims[c];
        }
    }

    auto getIdx = [&](const glm::dvec3 &centroid)
    {
        glm::dvec3 f = (centroid - centroid_extent.min) * inv_extent_dims;
        glm::ivec3 idx = glm::floor(f * glm::dvec3(num_bins));
        idx = glm::min(idx, num_bins - 1);
        return idx;
    };

    typedef std::pair<size_t, BoundingBox> bin;

    std::vector<std::vector<std::vector<bin>>> bins(num_bins[0], 
        std::vector<std::vector<bin>>(num_bins[1], 
            std::vector<bin>(num_bins[2], 
                { 0, BoundingBox() }
            )
        )
    );

    for (const auto &s : S)
    {
        glm::ivec3 idx = getIdx(s->midPoint());
        bins[idx.x][idx.y][idx.z].first++;
        bins[idx.x][idx.y][idx.z].second.merge(s->BB);
    }

    double min_cost = std::numeric_limits<double>::max();
    glm::ivec3 split_bin(0);

    for (size_t i = 0; i < num_bins[0] - 1; i++)
    {
        for (size_t j = 0; j < num_bins[1] - 1; j++)
        {
            for (size_t k = 0; k < num_bins[2] - 1; k++)
            {
                std::vector<BoundingBox> BBs(8, BoundingBox());
                std::vector<size_t> counts(8, 0);

                for (uint8_t v = 0b000; v <= 0b111; v++)
                {
                    std::vector<glm::ivec2> range(3, glm::ivec2(0));
                    range[0] = v & 0b001 ? glm::ivec2(i + 1, num_bins[0]) : glm::ivec2(0, i + 1);
                    range[1] = v & 0b010 ? glm::ivec2(j + 1, num_bins[1]) : glm::ivec2(0, j + 1);
                    range[2] = v & 0b100 ? glm::ivec2(k + 1, num_bins[2]) : glm::ivec2(0, k + 1);

                    for (size_t x = range[0][0]; x < range[0][1]; x++)
                    {
                        for (size_t y = range[1][0]; y < range[1][1]; y++)
                        {
                            for (size_t z = range[2][0]; z < range[2][1]; z++)
                            {
                                counts[v] += bins[x][y][z].first;
                                BBs[v].merge(bins[x][y][z].second);
                            }
                        }
                    }
                }

                double cost = 0.0;
                for (uint8_t v = 0b000; v <= 0b111; v++)
                {
                    cost += BBs[v].area() * counts[v];
                }

                cost = 1.0 + cost / bvh_node->BB.area();

                if (cost < min_cost)
                {
                    split_bin = glm::ivec3(i, j, k);
                    min_cost = cost;
                }
            }
        }
    }

    if (min_cost > S.size())
    {
        return;
    }

    std::vector<std::shared_ptr<BVHNode>> new_nodes(8, nullptr);

    for (const auto &s : S)
    {
        glm::ivec3 idx = getIdx(s->midPoint());

        uint8_t child_idx = 0b000;
        if (idx.x > split_bin.x) child_idx |= 0b001;
        if (idx.y > split_bin.y) child_idx |= 0b010;
        if (idx.z > split_bin.z) child_idx |= 0b100;

        if (!new_nodes[child_idx])
        {
            new_nodes[child_idx] = std::make_shared<BVHNode>();
        }

        new_nodes[child_idx]->surfaces.push_back(s);
        new_nodes[child_idx]->BB.merge(s->BB);
    }

    S.clear();

    size_t num_children = 0;

    for (const auto &child : new_nodes)
    {
        if (child)
        {
            num_children++;
            bvh_node->children.push_back(child);
            recursiveBuildOctonarySAH(child);
        }
    }
    branching[num_children]++;
}

double BVH::branchingFactor()
{
    double tot_branching = 0.0;
    double tot_branches = 0.0;
    for (const auto &b : branching)
    {
        tot_branching += b.second;
        tot_branches += b.second * b.first;
    }
    return tot_branches / tot_branching;
}


