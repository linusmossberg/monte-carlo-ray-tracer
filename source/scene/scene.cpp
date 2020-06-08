#include "scene.hpp"

#include "glm/gtx/component_wise.hpp"

#include "../ray/intersection.hpp"
#include "../common/util.hpp"
#include "../common/constants.hpp"

Scene::Scene(const nlohmann::json& j)
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

    generateEmissives();
}

Intersection Scene::intersect(const Ray& ray, bool align_normal, double min_distance) const
{
    bool use_stop_condition = min_distance > 0.0;

    Intersection intersect;
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
    if (align_normal && glm::dot(ray.direction, intersect.normal) > 0.0)
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

BoundingBox Scene::boundingBox(bool recompute)
{
    if (surfaces.empty()) return BoundingBox();

    if (recompute || !bounding_box)
    {
        BoundingBox bb = surfaces.front()->boundingBox();
        for (const auto& surface : surfaces)
        {
            auto s_bb = surface->boundingBox();
            for (uint8_t c = 0; c < 3; c++)
            {
                bb.min[c] = std::min(bb.min[c], s_bb.min[c]);
                bb.max[c] = std::max(bb.max[c], s_bb.max[c]);
            }
        }
        bounding_box = std::make_unique<BoundingBox>(bb);
    }

    return *bounding_box;
}

glm::dvec3 Scene::skyColor(const Ray& ray) const
{
    double f = (glm::dot(glm::dvec3(0.0, 1.0, 0.0), ray.direction) * 0.7 + 1.0) / 2.0;
    if (f < 0.5)
    {
        return glm::dvec3(0.5); // ground color
    }
    return glm::dvec3(1.0 - f, 0.5, f);
}