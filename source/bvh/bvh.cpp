#include "bvh.hpp"

#include <queue>
#include <chrono>
#include <iostream>

#include "../octree/octree.cpp"
#include "../common/format.hpp"
#include "../surface/surface.hpp"
#include "../common/util.hpp"

BVH::BVH(const BoundingBox &BB, 
         const std::vector<std::shared_ptr<Surface::Base>> &surfaces, 
         const nlohmann::json &j)
{
    df_idx = 0;

    std::shared_ptr<BuildNode> root = std::make_shared<BuildNode>();
    root->BB = BB;

    auto begin = std::chrono::high_resolution_clock::now();

    std::string type = getOptional<std::string>(j, "type", "OCTREE");
    std::transform(type.begin(), type.end(), type.begin(), toupper);

    if (type == "QUATERNARY_SAH")
    {
        bins_per_axis = getOptional(j, "bins_per_axis", 8);
        std::cout << "\nBuilding quaternary BVH using SAH.\n\n";
        root->surfaces = surfaces;
        recursiveBuildQuaternarySAH(root);
    }
    else if (type == "BINARY_SAH")
    {
        bins_per_axis = getOptional(j, "bins_per_axis", 16);
        std::cout << "\nBuilding binary BVH using SAH.\n\n";
        root->surfaces = surfaces;
        recursiveBuildBinarySAH(root);
    }
    else // OCTREE
    {
        std::cout << "\nBuilding BVH from octree.\n\n";

        double half_max = glm::compMax(root->BB.dimensions()) / 2.0;
        BoundingBox cube_BB(root->BB.centroid() - half_max, root->BB.centroid() + half_max);

        Octree<SurfaceCentroid> hierarchy(cube_BB, leaf_surfaces);

        for (const auto &s : surfaces)
        {
            hierarchy.insert(SurfaceCentroid(s));
        }

        recursiveBuildFromOctree(hierarchy, root);
    }

    size_t num_nodes = 1;
    double num_branchings = 0.0;
    for (const auto &b : branching)
    {
        num_branchings += b.second;
        num_nodes += b.first * b.second;
    }

    ordered_surfaces = std::vector<std::shared_ptr<Surface::Base>>(surfaces.size(), nullptr);

    linear_tree = std::vector<LinearNode>(num_nodes, LinearNode());

    uint32_t surface_idx = 0;
    compact(root, 0, surface_idx);

    auto end = std::chrono::high_resolution_clock::now();
    size_t msec_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

    std::cout << "BVH constructed in " + Format::timeDuration(msec_duration)
              << ". Branching factor of tree: " << (num_nodes - 1) / num_branchings << std::endl;
}

Intersection BVH::intersect(const Ray& ray)
{
    Intersection intersect;
    double t;
    if (linear_tree[0].BB.intersect(ray, t))
    {
        auto to_visit = reservedPriorityQueue<LinearNode::NodeIntersection>(64);

        uint32_t node_idx = 0;

        while (true)
        {
            const auto &node = linear_tree[node_idx];

            if (node.num_surfaces)
            {
                uint32_t end_idx = node.start_surface + node.num_surfaces;
                for (uint32_t i = node.start_surface; i < end_idx; i++)
                {
                    Intersection t_intersect;
                    if (ordered_surfaces[i]->intersect(ray, t_intersect))
                    {
                        if (t_intersect.t < intersect.t)
                        {
                            intersect = t_intersect;
                            intersect.surface = ordered_surfaces[i];
                        }
                    }
                }
            }
            else
            {
                uint32_t child_idx = node_idx + 1;
                while (child_idx != 0)
                {
                    if (linear_tree[child_idx].BB.intersect(ray, t) && t < intersect.t)
                    {
                        to_visit.emplace(child_idx, t);
                    }
                    child_idx = linear_tree[child_idx].next_sibling;
                }
            }

            if (to_visit.empty() || to_visit.top().t >= intersect.t)
            {
                break;
            }

            node_idx = to_visit.top().node; 
            to_visit.pop();
        }
    }
    return intersect;
}

void BVH::recursiveBuildFromOctree(const Octree<SurfaceCentroid> &octree_node, std::shared_ptr<BuildNode> bvh_node)
{
    bvh_node->df_idx = df_idx++;

    BoundingBox BB;

    if (octree_node.leaf())
    {
        bvh_node->surfaces.resize(octree_node.data_vec.size());
        for (size_t i = 0; i < bvh_node->surfaces.size(); i++)
        {
            bvh_node->surfaces[i] = octree_node.data_vec[i].surface;
            BB.merge(bvh_node->surfaces[i]->BB());
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
    bvh_node->df_idx = df_idx++;

    auto &S = bvh_node->surfaces;

    if (S.size() <= leaf_surfaces)
    {
        return;
    }

    BoundingBox centroid_extent;
    for (const auto &s : S)
    {
        centroid_extent.merge(s->BB().centroid());
    }
    glm::dvec3 extent_dims = centroid_extent.dimensions();

    uint8_t split_axis = extent_dims.x > extent_dims.y ? 
                        (extent_dims.x > extent_dims.z ? 0 : 2) : 
                        (extent_dims.y > extent_dims.z ? 1 : 2);

    if (extent_dims[split_axis] < C::EPSILON)
    {
        if (S.size() > max_leaf_surfaces)
        {
            arbitrarySplit(bvh_node, 2);
            for (const auto& child : bvh_node->children) recursiveBuildBinarySAH(child);
        }
        return;
    }

    auto getIdx = [&](const glm::dvec3 &centroid)
    {
        double f = (centroid[split_axis] - centroid_extent.min[split_axis]) / extent_dims[split_axis];
        int idx = (int)glm::floor(f * bins_per_axis);
        return glm::min(idx, bins_per_axis - 1);
    };

    std::vector<std::pair<size_t, BoundingBox>> bins(bins_per_axis, { 0, BoundingBox() });
    for (const auto &s : S)
    {
        int idx = getIdx(s->BB().centroid());
        bins[idx].first++;
        bins[idx].second.merge(s->BB());
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
        for (size_t j = i + 1; j < bins_per_axis; j++)
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
        if (S.size() > max_leaf_surfaces)
        {
            arbitrarySplit(bvh_node, 2);
            for (const auto& child : bvh_node->children) recursiveBuildBinarySAH(child);
        }
        return;
    }

    for (const auto &s : S)
    {
        int idx = getIdx(s->BB().centroid());
        if (idx <= split_bin)
        {
            A->surfaces.push_back(s);
            A->BB.merge(s->BB());
        }
        else
        {
            B->surfaces.push_back(s);
            B->BB.merge(s->BB());
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
    bvh_node->df_idx = df_idx++;

    glm::ivec2 num_bins(bins_per_axis);

    auto &S = bvh_node->surfaces;

    if (S.size() <= leaf_surfaces)
    {
        return;
    }

    BoundingBox centroid_extent;
    for (const auto &s : S)
    {
        centroid_extent.merge(s->BB().centroid());
    }
    glm::dvec3 extent_dims = centroid_extent.dimensions();

    glm::ivec2 axes = extent_dims.x > extent_dims.y ?
                     (extent_dims.y > extent_dims.z ? glm::ivec2(0, 1) : glm::ivec2(0, 2)) :
                     (extent_dims.x > extent_dims.z ? glm::ivec2(0, 1) : glm::ivec2(1, 2));
    
    if (extent_dims[axes.x] < C::EPSILON || extent_dims[axes.y] < C::EPSILON)
    {
        df_idx--;
        recursiveBuildBinarySAH(bvh_node);
        return;
    }

    glm::dvec2 axes_min(centroid_extent.min[axes.x], centroid_extent.min[axes.y]);
    glm::dvec2 axes_dim(extent_dims[axes.x], extent_dims[axes.y]);

    auto getIdx = [&](const glm::dvec3 &centroid)
    {
        glm::dvec2 f = (glm::dvec2(centroid[axes.x], centroid[axes.y]) - axes_min) / axes_dim;
        glm::ivec2 idx = glm::floor(f * glm::dvec2(num_bins));
        return glm::min(idx, num_bins - 1);
    };

    std::vector<std::vector<std::pair<size_t, BoundingBox>>> bins(num_bins.x,
        std::vector<std::pair<size_t, BoundingBox>>(num_bins.y, { 0, BoundingBox() })
    );

    for (const auto &s : S)
    {
        glm::ivec2 idx = getIdx(s->BB().centroid());
        bins[idx.x][idx.y].first++;
        bins[idx.x][idx.y].second.merge(s->BB());
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
        if (S.size() > max_leaf_surfaces)
        {
            arbitrarySplit(bvh_node, 4);
            for (const auto& child : bvh_node->children) recursiveBuildQuaternarySAH(child);
        }
        return;
    }

    std::vector<std::shared_ptr<BuildNode>> new_nodes(4, nullptr);

    for (const auto &s : S)
    {
        glm::ivec2 idx = getIdx(s->BB().centroid());

        uint8_t child_idx = 0b00;
        if (idx.x > split_bin.x) child_idx |= 0b01;
        if (idx.y > split_bin.y) child_idx |= 0b10;

        if (!new_nodes[child_idx])
        {
            new_nodes[child_idx] = std::make_shared<BuildNode>();
        }

        new_nodes[child_idx]->surfaces.push_back(s);
        new_nodes[child_idx]->BB.merge(s->BB());
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

void BVH::compact(std::shared_ptr<BuildNode> bvh_node, uint32_t next_sibling, uint32_t &surface_idx)
{
    linear_tree[bvh_node->df_idx].BB = bvh_node->BB;
    linear_tree[bvh_node->df_idx].next_sibling = next_sibling;
    linear_tree[bvh_node->df_idx].start_surface = surface_idx;
    linear_tree[bvh_node->df_idx].num_surfaces = (uint8_t)bvh_node->surfaces.size();

    for (const auto &surface : bvh_node->surfaces)
    {
        ordered_surfaces[surface_idx] = surface;
        surface_idx++;
    }

    if (!bvh_node->children.empty())
    {
        for (size_t i = 0; i < bvh_node->children.size() - 1; i++)
        {
            compact(bvh_node->children[i], bvh_node->children[i + 1]->df_idx, surface_idx);
        }
        compact(bvh_node->children.back(), 0, surface_idx);
    }
}

void BVH::arbitrarySplit(std::shared_ptr<BuildNode> bvh_node, size_t N)
{
    auto& S = bvh_node->surfaces;

    N = std::min(N, S.size());

    for (int i = 0; i < N; i++)
    {
        bvh_node->children.push_back(std::make_shared<BuildNode>());
    }

    for (size_t i = 0; i < S.size(); i++)
    {
        size_t idx = i % N;

        bvh_node->children[idx]->surfaces.push_back(S[i]);
        bvh_node->children[idx]->BB.merge(S[i]->BB());
    }

    S.clear();

    branching[N]++;
}

BVH::SurfaceCentroid::SurfaceCentroid(std::shared_ptr<Surface::Base> surface)
    : surface(surface), centroid(surface->BB().centroid()) { }