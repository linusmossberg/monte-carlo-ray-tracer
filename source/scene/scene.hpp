#pragma once

#include <vector>
#include <thread>
#include <memory>

#include <glm/gtx/norm.hpp>
#include <nlohmann/json.hpp>

#include "../surface/surface.hpp"
#include "../ray/ray.hpp"
#include "../random/random.hpp"
#include "../common/intersection.hpp"
#include "../common/bounding-box.hpp"

class Scene
{
public:
    Scene(const nlohmann::json& j)
    {
        std::unordered_map<std::string, std::shared_ptr<Material>> materials = j.at("materials");
        std::unordered_map<std::string, std::vector<glm::dvec3>> vertices = j.at("vertices");

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
                const auto& v = vertices.at(s.at("vertex_set"));

                bool is_emissive = glm::compMax(material->emittance) > C::EPSILON;
                double total_area = 0.0;
                if (is_emissive)
                {
                    for (const auto& t : s.at("triangles"))
                    {
                        total_area += Surface::Triangle(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), material).area();
                    }
                }

                for (const auto& t : s.at("triangles"))
                {
                    // Entire object emits the flux of assigned material emittance in scene file.
                    // The flux of the material therefore needs to be distributed amongst all object triangles.
                    if (is_emissive && total_area > C::EPSILON)
                    {
                        double area = Surface::Triangle(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), material).area();
                        auto new_mat = std::make_shared<Material>(*material);
                        new_mat->emittance *= area / total_area;
                        surfaces.push_back(std::make_shared<Surface::Triangle>(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), new_mat));
                    }
                    else
                    {
                        surfaces.push_back(std::make_shared<Surface::Triangle>(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), material));
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
        }

        generateEmissives();
    }

    Intersection intersect(const Ray& ray, bool align_normal = false, double min_distance = -1) const;

    void generateEmissives();

    BoundingBox boundingBox(bool recompute);

    glm::dvec3 skyColor(const Ray& ray) const;

    std::vector<std::shared_ptr<Surface::Base>> surfaces;
    std::vector<std::shared_ptr<Surface::Base>> emissives; // subset of surfaces

    double ior;

protected:
    std::unique_ptr<BoundingBox> bounding_box;
};