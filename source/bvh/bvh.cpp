#include "bvh.hpp"

#include <queue>
#include <chrono>
#include <iostream>

#include "../octree/octree.cpp"
#include "../common/format.hpp"

BVH::BVH(const BoundingBox &BB, 
         const std::vector<std::shared_ptr<Surface::Base>> &surfaces, 
         const nlohmann::json &j)
{
    df_idx = 0;
    std::shared_ptr<BuildNode> root = std::make_shared<BuildNode>();
    root->BB = BB;
    root->BB.computeProperties();

    auto begin = std::chrono::high_resolution_clock::now();

    std::string type = getOptional<std::string>(j, "type", "QUATERNARY_SAH");
    std::transform(type.begin(), type.end(), type.begin(), toupper);

    if (type == "OCTREE")
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
    }
    else if (type == "BINARY_SAH")
    {
        bins_per_axis = getOptional(j, "bins_per_axis", 16);
        std::cout << "\nBuilding binary BVH using SAH.\n\n";
        root->surfaces = surfaces;
        recursiveBuildBinarySAH(root);
    }
    else // QUATERNARY_SAH
    {
        bins_per_axis = getOptional(j, "bins_per_axis", 8);
        std::cout << "\nBuilding quaternary BVH using SAH.\n\n";
        root->surfaces = surfaces;
        recursiveBuildQuaternarySAH(root);
    }


    size_t num_nodes = 0;
    double num_branchings = 0.0;
    for (const auto &b : branching)
    {
        num_branchings += b.second;
        num_nodes += b.first * b.second;
    }

    linear_tree = std::vector<LinearNode>(num_nodes, LinearNode());

    compact(root, 0);

    auto end = std::chrono::high_resolution_clock::now();
    size_t msec_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "BVH constructed in " + Format::timeDuration(msec_duration)
              << ". Branching factor of tree: " << num_nodes / num_branchings << std::endl << std::endl;
}

Intersection BVH::intersect(const Ray& ray)
{
    Intersection intersect;
    double t;
    if (linear_tree[0].BB.intersect(ray, t))
    {
        std::priority_queue<LinearNode::NodeIntersection> to_visit;
        to_visit.emplace(0, t);
        uint32_t current_node;

        while (!to_visit.empty() && to_visit.top().t < intersect.t)
        {
            current_node = to_visit.top().node;
            to_visit.pop();

            if (linear_tree[current_node].leaf)
            {
                for (const auto& surface : linear_tree[current_node].surfaces)
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
                int32_t child_node = current_node + 1;
                while (child_node >= 0)
                {
                    if (linear_tree[child_node].BB.intersect(ray, t) && t < intersect.t)
                    {
                        to_visit.emplace(child_node, t);
                    }
                    child_node = linear_tree[child_node].right_sibling;
                }
            }
        }
    }
    return intersect;
}

void BVH::recursiveBuildFromOctree(const Octree<SurfacePoint> &octree_node, std::shared_ptr<BuildNode> bvh_node)
{
    bvh_node->df_idx = df_idx; df_idx++;

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
                num_children++;
                std::shared_ptr<BuildNode> child = std::make_shared<BuildNode>();
                bvh_node->children.push_back(child);
                recursiveBuildFromOctree(*octree_node.octants[i], child);
                BB.merge(child->BB);
                
            }
        }
        branching[num_children]++;
    }
    bvh_node->BB = BB;
}

void BVH::recursiveBuildBinarySAH(std::shared_ptr<BuildNode> bvh_node)
{
    bvh_node->df_idx = df_idx; df_idx++;

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

    uint8_t split_axis = extent_dims.x > extent_dims.y ? 
                        (extent_dims.x > extent_dims.z ? 0 : 2) : 
                        (extent_dims.y > extent_dims.z ? 1 : 2);

    if (extent_dims[split_axis] <= 0.0)
    {
        return;
    }

    auto getIdx = [&](const glm::dvec3 &centroid)
    {
        double f = (centroid[split_axis] - centroid_extent.min[split_axis]) / extent_dims[split_axis];
        int idx = glm::floor(f * bins_per_axis);
        idx = glm::min(idx, bins_per_axis - 1);
        return idx;
    };

    std::vector<std::pair<size_t, BoundingBox>> bins(bins_per_axis, { 0, BoundingBox() });
    for (const auto &s : S)
    {
        int idx = getIdx(s->midPoint());
        bins[idx].first++;
        bins[idx].second.merge(s->BB);
    }

    std::shared_ptr<BuildNode> A = std::make_shared<BuildNode>();
    std::shared_ptr<BuildNode> B = std::make_shared<BuildNode>();

    double min_cost = std::numeric_limits<double>::max();
    size_t split_bin = 0;

    for (size_t i = 0; i < bins_per_axis - 1; i++)
    {
        size_t A_count = 0;
        BoundingBox A_BB;
        for (size_t j = 0; j < i + 1; j++)
        {
            A_count += bins[j].first;
            A_BB.merge(bins[j].second);
        }

        size_t B_count = 0;
        BoundingBox B_BB;
        for (size_t j = i + 1; j < bins.size(); j++)
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

    S.clear();
   
    size_t num_children = 0;

    if (!A->surfaces.empty())
    {
        num_children++;
        bvh_node->children.push_back(A);
        recursiveBuildBinarySAH(A);
    }
    if (!B->surfaces.empty())
    {
        num_children++;
        bvh_node->children.push_back(B);
        recursiveBuildBinarySAH(B);
    }

    branching[num_children]++;
}

void BVH::recursiveBuildQuaternarySAH(std::shared_ptr<BuildNode> bvh_node)
{
    bvh_node->df_idx = df_idx; df_idx++;

    glm::ivec2 num_bins(bins_per_axis);

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

    glm::ivec2 axes = extent_dims.x > extent_dims.y ?
                     (extent_dims.y > extent_dims.z ? glm::ivec2(0, 1) : glm::ivec2(0, 2)) :
                     (extent_dims.x > extent_dims.z ? glm::ivec2(0, 1) : glm::ivec2(1, 2));
    
    if (extent_dims[axes.x] <= 0.0 || extent_dims[axes.y] <= 0.0)
    {
        return;
    }

    glm::dvec2 axes_min(centroid_extent.min[axes.x], centroid_extent.min[axes.y]);
    glm::dvec2 axes_dim(extent_dims[axes.x], extent_dims[axes.y]);

    auto getIdx = [&](const glm::dvec3 &centroid)
    {
        glm::dvec2 f = (glm::dvec2(centroid[axes.x], centroid[axes.y]) - axes_min) / axes_dim;
        glm::ivec2 idx = glm::floor(f * glm::dvec2(num_bins));
        idx = glm::min(idx, num_bins - 1);
        return idx;
    };

    std::vector<std::vector<std::pair<size_t, BoundingBox>>> bins(num_bins.x,
        std::vector<std::pair<size_t, BoundingBox>>(num_bins.y, { 0, BoundingBox() })
    );

    for (const auto &s : S)
    {
        glm::ivec2 idx = getIdx(s->midPoint());
        bins[idx.x][idx.y].first++;
        bins[idx.x][idx.y].second.merge(s->BB);
    }

    double min_cost = std::numeric_limits<double>::max();
    glm::ivec2 split_bin(0);

    for (size_t i = 0; i < num_bins.x - 1; i++)
    {
        for (size_t j = 0; j < num_bins.y - 1; j++)
        {
            std::vector<BoundingBox> BBs(4, BoundingBox());
            std::vector<size_t> counts(4, 0);

            for (uint8_t v = 0b00; v <= 0b11; v++)
            {
                std::vector<glm::ivec2> range(2, glm::ivec2(0));
                range[0] = v & 0b01 ? glm::ivec2(i + 1, num_bins.x) : glm::ivec2(0, i + 1);
                range[1] = v & 0b10 ? glm::ivec2(j + 1, num_bins.y) : glm::ivec2(0, j + 1);

                for (size_t x = range[0][0]; x < range[0][1]; x++)
                {
                    for (size_t y = range[1][0]; y < range[1][1]; y++)
                    {
                        counts[v] += bins[x][y].first;
                        BBs[v].merge(bins[x][y].second);
                    }
                }
            }

            double cost = 0.0;
            for (uint8_t v = 0b00; v <= 0b11; v++)
            {
                cost += BBs[v].area() * counts[v];
            }

            cost = 1.0 + cost / bvh_node->BB.area();

            if (cost < min_cost)
            {
                split_bin = glm::ivec2(i, j);
                min_cost = cost;
            }
        }
    }

    if (min_cost > S.size())
    {
        return;
    }

    std::vector<std::shared_ptr<BuildNode>> new_nodes(4, nullptr);

    for (const auto &s : S)
    {
        glm::ivec2 idx = getIdx(s->midPoint());

        uint8_t child_idx = 0b00;
        if (idx.x > split_bin.x) child_idx |= 0b01;
        if (idx.y > split_bin.y) child_idx |= 0b10;

        if (!new_nodes[child_idx])
        {
            new_nodes[child_idx] = std::make_shared<BuildNode>();
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
            recursiveBuildQuaternarySAH(child);
        }
    }
    branching[num_children]++;
}

void BVH::compact(std::shared_ptr<BuildNode> bvh_node, int32_t right_sibling)
{
    linear_tree[bvh_node->df_idx].BB = bvh_node->BB;
    linear_tree[bvh_node->df_idx].surfaces = bvh_node->surfaces;
    linear_tree[bvh_node->df_idx].right_sibling = right_sibling;
    linear_tree[bvh_node->df_idx].leaf = bvh_node->leaf();

    for (size_t i = 0; i < bvh_node->children.size(); i++)
    {
        if (i == bvh_node->children.size() - 1)
        {
            compact(bvh_node->children[i], -1);
        }
        else
        {
            compact(bvh_node->children[i], bvh_node->children[i + 1]->df_idx);
        }
    }
}