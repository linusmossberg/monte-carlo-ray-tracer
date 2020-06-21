#include "scene.hpp"

#include "glm/gtx/component_wise.hpp"

#include "../ray/intersection.hpp"
#include "../common/util.hpp"
#include "../common/constants.hpp"
#include "../common/format.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

Scene::Scene(const nlohmann::json& j)
{
    std::unordered_map<std::string, std::shared_ptr<Material>> materials = j.at("materials");
    auto vertices = getOptional(j, "vertices", std::unordered_map<std::string, std::vector<glm::dvec3>>());

    ior = j.at("ior");

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
            std::vector<glm::dvec3> v;
            std::vector<std::vector<size_t>> triangles;
            if (s.find("file") != s.end())
            {
                auto path = std::filesystem::current_path() / "scenes";
                path /= s.at("file").get<std::string>();
                parseOBJ(path, v, triangles);
            }
            else
            {
                v = vertices.at(s.at("vertex_set"));
                triangles = s.at("triangles").get<std::vector<std::vector<size_t>>>();
            }

            bool is_emissive = glm::compMax(material->emittance) > C::EPSILON;
            double total_area = 0.0;
            if (is_emissive)
            {
                for (const auto& t : triangles)
                {
                    total_area += Surface::Triangle(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), material).area();
                }
            }

            for (const auto& t : triangles)
            {
                // Entire object emits the flux of assigned material emittance in scene file.
                // The flux of the material therefore needs to be distributed amongst all object triangles.
                std::shared_ptr<Material> mat;
                if (is_emissive && total_area > C::EPSILON)
                {
                    double area = Surface::Triangle(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), material).area();
                    mat = std::make_shared<Material>(*material);
                    mat->emittance *= area / total_area;
                }
                else
                {
                    mat = material;
                }

                surfaces.push_back(std::make_shared<Surface::Triangle>(
                    v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), mat)
                );
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
        bvh = std::make_unique<BVH>(BB_, surfaces, j.at("bvh"));
    }

    generateEmissives();
}

Intersection Scene::intersect(const Ray& ray, bool align_normal, double min_distance) const
{
    Intersection intersect;

    if (bvh)
    {
        intersect = bvh->intersect(ray);
    }
    else
    {
        bool use_stop_condition = min_distance > 0.0;

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
    }

    if (align_normal && intersect && glm::dot(ray.direction, intersect.normal) > 0.0)
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

void Scene::computeBoundingBox()
{
    for (const auto& surface : surfaces)
    {
        BB_.merge(surface->BB());
    }
}

glm::dvec3 Scene::skyColor(const Ray& ray) const
{
    double f = (1.0 + glm::dot(glm::dvec3(0.0, 1.0, 0.0), ray.direction)) / 2.0;
    //if (f < 0.2) return glm::dvec3(0.1);
    return glm::mix(glm::dvec3(1.0, 0.5, 0.0), glm::dvec3(0.0, 0.5, 1.0), f);
}

void Scene::parseOBJ(const std::filesystem::path &path, 
                     std::vector<glm::dvec3> &vertices,
                     std::vector<std::vector<size_t>> &triangles) const
{
    std::ifstream file(path);

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
        else if (line_type == "f")
        {
            std::vector<size_t> triangle(3);
            for (int i = 0; i < 3; i++)
            {
                std::string element;
                ss >> element;
                if (ss.peek() == '-')
                {
                    throw std::runtime_error("OBJ files with negative vertex offsets are not supported.");
                }
                triangle[i] = std::stoull(element.substr(0, element.find("/", 0))) - 1;
            }
            triangles.push_back(triangle);
        }
    }

    file.close();
}