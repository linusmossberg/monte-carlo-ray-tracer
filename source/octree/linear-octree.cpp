#include "linear-octree.hpp"

#include <glm/gtx/norm.hpp>

#include "../common/util.hpp"

template <class Data>
LinearOctree<Data>::LinearOctree(Octree<Data> &octree_root)
{
    size_t octree_size = 0, data_size = 0;
    octreeSize(octree_root, octree_size, data_size);

    if (octree_size == 0 || data_size == 0) return;

    linear_tree = std::vector<LinearOctant>(octree_size, LinearOctant());
    ordered_data.reserve(data_size);

    int32_t df_idx = 0;
    uint64_t data_idx = 0;
    compact(octree_root, df_idx, data_idx, true);

    octree_root.octants.clear(); 
}

template <class Data>
std::vector<SearchResult<Data>> LinearOctree<Data>::knnSearch(const glm::dvec3& p, size_t k, double max_distance) const
{
    std::vector<SearchResult<Data>> result;

    if (linear_tree.empty()) return result;

    double max_distance2 = pow2(max_distance);
    double distance2 = linear_tree[0].BB.distance2(p);

    if (distance2 > max_distance2)
    {
        return result;
    }

    std::priority_queue<KNNode> to_visit;

    KNNode current(0, distance2);

    while (true)
    {
        if (current.octant >= 0)
        {
            if (linear_tree[current.octant].leaf)
            {
                uint64_t end_idx = linear_tree[current.octant].start_data + linear_tree[current.octant].num_data;
                for (uint64_t i = linear_tree[current.octant].start_data; i < end_idx; i++)
                {
                    double distance2 = glm::distance2(ordered_data[i].pos(), p);
                    if (distance2 <= max_distance2)
                    {
                        to_visit.emplace(i, distance2);
                    }
                }
            }
            else
            {
                int32_t child_octant = current.octant + 1;
                while (child_octant > 0)
                {
                    double distance2 = linear_tree[child_octant].BB.distance2(p);
                    if (distance2 <= max_distance2)
                    {
                        to_visit.emplace(child_octant, distance2);
                    }
                    child_octant = linear_tree[child_octant].next_sibling;
                }
            }
        }
        else
        {
            result.emplace_back(ordered_data[current.data], current.distance2);

            if (result.size() == k) return result;
        }

        if (to_visit.empty()) return result;

        current = to_visit.top();
        to_visit.pop();
    }
}

template <class Data>
void LinearOctree<Data>::octreeSize(const Octree<Data> &octree_root, size_t &size, size_t &data_size) const
{
    if (octree_root.leaf() && octree_root.data_vec.empty()) return;

    size++;
    data_size += octree_root.data_vec.size();

    if (!octree_root.leaf())
    {
        for (const auto &octant : octree_root.octants)
        {
            octreeSize(*octant, size, data_size);
        }
    }
}

template <class Data>
void LinearOctree<Data>::compact(Octree<Data> &node, int32_t &df_idx, uint64_t &data_idx, bool last)
{
    int32_t idx = df_idx++;

    linear_tree[idx].BB = node.BB;
    linear_tree[idx].leaf = (uint8_t)node.leaf();
    linear_tree[idx].start_data = data_idx;
    linear_tree[idx].num_data = (uint16_t)node.data_vec.size();

    for (const auto &data : node.data_vec)
    {
        ordered_data.push_back(data);
        data_idx++;
    }

    node.data_vec.clear();

    if (!node.leaf())
    {
        std::vector<size_t> use;
        for (size_t i = 0; i < node.octants.size(); i++)
        {
            if (!(node.octants[i]->leaf() && node.octants[i]->data_vec.empty()))
            {
                use.push_back(i);
            }
        }
        for (const auto &i : use)
        {
            compact(*node.octants[i], df_idx, data_idx, i == use.back());
        }
    }
    linear_tree[idx].next_sibling = last ? -1 : df_idx;
}
