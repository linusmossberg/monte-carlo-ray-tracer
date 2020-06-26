#include "scene.hpp"

#include "glm/gtx/component_wise.hpp"

#include "../ray/intersection.hpp"
#include "../common/util.hpp"
#include "../common/constants.hpp"
#include "../common/format.hpp"
#include "../material/material.hpp"
#include "../surface/surface.hpp"
#include "../bvh/bvh.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

Scene::Scene(const nlohmann::json& j)
{
    std::unordered_map<std::string, std::shared_ptr<Material>> materials = j.at("materials");
    auto vertices = getOptional(j, "vertices", std::unordered_map<std::string, std::vector<glm::dvec3>>());
    ior = getOptional(j, "ior", 1.0);

    for (const auto& s : j.at("surfaces"))
    {
        std::string material_str = "default";
        if (s.find("material") != s.end())
        {
            material_str = s.at("material");
        }
        auto& material = materials.at(material_str);

        std::string type = s.at("type");
        if (type == "object")
        {
            std::vector<glm::dvec3> v, n;
            std::vector<std::vector<size_t>> triangles_v, triangles_vt, triangles_vn;
            if (s.find("file") != s.end())
            {
                auto path = std::filesystem::current_path() / "scenes";
                path /= s.at("file").get<std::string>();
                parseOBJ(path, v, n, triangles_v, triangles_vt, triangles_vn);
            }
            else
            {
                v = vertices.at(s.at("vertex_set"));
                triangles_v = s.at("triangles").get<std::vector<std::vector<size_t>>>();
            }

            bool smooth = getOptional(s, "smooth", false);

            if (smooth && n.empty())
            {
                generateVertexNormals(n, v, triangles_v);
                triangles_vn = triangles_v;
            }

            bool is_emissive = glm::compMax(material->emittance) > C::EPSILON;
            double total_area = 0.0;
            if (is_emissive)
            {
                for (const auto& t : triangles_v)
                {
                    total_area += Surface::Triangle(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), nullptr).area();
                }
            }

            for (size_t i = 0; i < triangles_v.size(); i++)
            {
                const auto &t = triangles_v[i];

                // Entire object emits the flux of assigned material emittance in scene file.
                // The flux of the material therefore needs to be distributed amongst all object triangles.
                std::shared_ptr<Material> mat;
                if (is_emissive && total_area > C::EPSILON)
                {
                    double area = Surface::Triangle(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), nullptr).area();
                    mat = std::make_shared<Material>(*material);
                    mat->emittance *= area / total_area;
                }
                else
                {
                    mat = material;
                }

                if (smooth)
                {
                    const auto &tn = triangles_vn[i];
                    surfaces.push_back(std::make_shared<Surface::Triangle>(
                        v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)),
                        n.at(tn.at(0)), n.at(tn.at(1)), n.at(tn.at(2)), mat)
                    );
                }
                else
                {
                    surfaces.push_back(std::make_shared<Surface::Triangle>(
                        v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), mat)
                    );
                }
            }
        }
        else if (type == "triangle")
        {
            const auto& v = s.at("vertices");
            surfaces.push_back(std::make_shared<Surface::Triangle>(v.at(0), v.at(1), v.at(2), material));
        }
        else if (type == "sphere")
        {
            surfaces.push_back(std::make_shared<Surface::Sphere>(s.at("origin"), s.at("radius"), material));
        }
        else if (type == "quadric")
        {
            // Emittance is not supported for general quadrics 
            // (no parameterization -> no uniform surface sampling or surface area integral)
            std::shared_ptr<Material> mat = material;
            if (glm::compMax(material->emittance) > C::EPSILON)
            {
                mat = std::make_shared<Material>(*material);
                mat->emittance = glm::dvec3(0.0);
            }
            surfaces.push_back(std::make_shared<Surface::Quadric>(s, mat));
        }
    }

    computeBoundingBox();

    std::cout << "\nNumber of primitives: " << Format::largeNumber(surfaces.size()) << std::endl;

    if (j.find("bvh") != j.end())
    {
        bvh = std::make_shared<BVH>(BB_, surfaces, j.at("bvh"));
    }

    generateEmissives();
}

Interaction Scene::interact(const Ray& ray, bool shadow_ray) const
{
    Intersection intersection;

    if (bvh)
    {
        intersection = bvh->intersect(ray);
    }
    else
    {
        for (const auto& s : surfaces)
        {
            Intersection t_intersection;
            if (s->intersect(ray, t_intersection))
            {
                if (t_intersection.t < intersection.t)
                {
                    intersection = t_intersection;
                    intersection.surface = s;
                }
            }
        }
    }

    Interaction interaction;

    if (intersection)
    {
        interaction.position = ray(intersection.t);
        interaction.normal = intersection.surface->normal(interaction.position);
        interaction.material = intersection.surface->material;
        interaction.t = intersection.t;

        if (!shadow_ray)
        {
            double cos_theta = glm::dot(ray.direction, interaction.normal);

            if (intersection.interpolate)
            {
                interaction.shading_normal = intersection.surface->interpolatedNormal(intersection.uv);
                if (cos_theta < 0.0 != glm::dot(ray.direction, interaction.shading_normal) < 0.0)
                {
                    interaction.shading_normal = interaction.normal;
                }
            }
            else
            {
                interaction.shading_normal = interaction.normal;
            }

            if (cos_theta > 0.0)
            {
                interaction.normal = -interaction.normal;
                interaction.shading_normal = -interaction.shading_normal;
            }
        }
    }
    return interaction;
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

void Scene::computeBoundingBox()
{
    for (const auto& surface : surfaces)
    {
        BB_.merge(surface->BB());
    }
}

glm::dvec3 Scene::skyColor(const Ray& ray) const
{
    double fy = (1.0 + std::asin(glm::dot(glm::dvec3(0.0, 1.0, 0.0), ray.direction)) / C::PI) / 2.0;
    return glm::mix(glm::dvec3(1.0, 0.5, 0.0), glm::dvec3(0.0, 0.5, 1.0), fy);
}

void Scene::parseOBJ(const std::filesystem::path &path, 
                     std::vector<glm::dvec3> &vertices,
                     std::vector<glm::dvec3> &normals,
                     std::vector<std::vector<size_t>> &triangles_v,
                     std::vector<std::vector<size_t>> &triangles_vt,
                     std::vector<std::vector<size_t>> &triangles_vn) const
{
    std::ifstream file(path);

    auto isNumber = [](const std::string &s) 
    {
        return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
    };

    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string line_type;
        ss >> line_type;

        if (line_type == "v")
        {
            glm::dvec3 v;
            ss >> v.x >> v.y >> v.z;

            vertices.push_back(v);
        }
        if (line_type == "vn")
        {
            glm::dvec3 vn;
            ss >> vn.x >> vn.y >> vn.z;

            normals.push_back(vn);
        }
        else if (line_type == "f")
        {
            std::vector<size_t> triangle_v, triangle_vt, triangle_vn;
            for (int i = 0; i < 3; i++)
            {
                std::string element;
                ss >> element;
                std::stringstream ss_f(element);

                std::vector<size_t> idxs;
                while (ss_f.good())
                {
                    if (ss_f.peek() == '-')
                    {
                        throw std::runtime_error("OBJ files with negative offsets are not supported.");
                    }
                    std::string idx;
                    std::getline(ss_f, idx, '/');
                    idxs.push_back(isNumber(idx) ? std::stoull(idx) : 0);
                }

                if (idxs.size() == 1)
                {
                    triangle_v.push_back(idxs[0] - 1);
                }
                else if (idxs.size() == 2)
                {
                    triangle_v.push_back(idxs[0] - 1);
                    triangle_vt.push_back(idxs[1] - 1);
                }
                else if (idxs.size() == 3)
                {
                    triangle_v.push_back(idxs[0] - 1);
                    if (idxs[1]) triangle_vt.push_back(idxs[1] - 1);
                    triangle_vn.push_back(idxs[2] - 1);
                }
            }
            if (triangle_v.size() == 3) triangles_v.push_back(triangle_v);
            if (triangle_vt.size() == 3) triangles_vt.push_back(triangle_vt);
            if (triangle_vn.size() == 3) triangles_vn.push_back(triangle_vn);
        }
    }

    file.close();
}

void Scene::generateVertexNormals(std::vector<glm::dvec3> &normals,
                                  const std::vector<glm::dvec3> &vertices,
                                  const std::vector<std::vector<size_t>> &triangles) const
{
    normals.resize(vertices.size(), glm::dvec3(0.0));

    for (const auto &t : triangles)
    {
        auto triangle = Surface::Triangle(vertices.at(t.at(0)), vertices.at(t.at(1)), vertices.at(t.at(2)), nullptr);

        glm::dvec3 weighted_normal = triangle.normal() * triangle.area();

        normals.at(t.at(0)) += weighted_normal;
        normals.at(t.at(1)) += weighted_normal;
        normals.at(t.at(2)) += weighted_normal;
    }

    for (auto &n : normals)
    {
        n = glm::normalize(n);
    }
}