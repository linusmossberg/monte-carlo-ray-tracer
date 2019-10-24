#pragma once

#include <fstream>
#include <filesystem>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "scene-renderer.hpp"

inline glm::dvec3 j2v(const nlohmann::json &j)
{
    return glm::dvec3(j.at(0), j.at(1), j.at(2));
}

template <class T>
inline T getOptional(const nlohmann::json &j, std::string value, T default_value)
{
    T ret = default_value;
    if (j.find(value) != j.end())
    {
        ret = j.at(value);
    }
    return ret;
}

inline glm::dvec3 getOptional(const nlohmann::json &j, std::string value, const glm::dvec3 &default_value)
{
    glm::dvec3 ret = default_value;
    if (j.find(value) != j.end())
    {
        ret = j2v(j.at(value));
    }
    return ret;
}

class SceneParser
{
public:
    static std::vector<SceneOption> availible(std::filesystem::path path)
    {
        std::vector<SceneOption> options;
        for (const auto& file : std::filesystem::directory_iterator(path))
        {
            if (!file.path().has_extension() || file.path().extension() != ".json")
                continue;

            std::ifstream scene_file(file.path());
            nlohmann::json j;
            scene_file >> j;
            int i = 0;
            for (const auto& camera : j.at("cameras"))
            {
                glm::dvec3 eye = j2v(camera.at("eye"));
                double f = camera.at("focal_length");
                double s = camera.at("sensor_width");
                std::stringstream ss;
                ss << "Eye: " << std::fixed << std::setprecision(0) << "(" << eye.x << " " << eye.y << " " << eye.z << "), ";
                ss << "Focal length: " << int(f) << "mm (" << int(s) << "mm)";
                options.emplace_back(file.path(), ss.str(), i);
                i++;
            }
            scene_file.close();
        }
        return options;
    }

    static SceneRenderer parseScene(const std::filesystem::path path, int camera_idx)
    {
        std::ifstream scene_file(path);
        nlohmann::json j;
        scene_file >> j;

        const auto& c = j.at("cameras").at(camera_idx);

        std::shared_ptr<Camera> camera;
        if (c.find("look_at") != c.end())
        {
            camera = std::make_shared<Camera>(
                j2v(c.at("eye")), j2v(c.at("look_at")), 
                c.at("focal_length"), c.at("sensor_width"), 
                c.at("width"), c.at("height")
            );
        }
        else
        {
            camera = std::make_shared<Camera>(
                j2v(c.at("eye")), j2v(c.at("forward")), j2v(c.at("up")), 
                c.at("focal_length"), c.at("sensor_width"), 
                c.at("width"), c.at("height")
            );
        }
        camera->setNaive(getOptional(j, "naive", false));

        double scene_ior = j.at("ior");

        std::unordered_map<std::string, std::shared_ptr<Material>> materials;
        for (const auto& m : j.at("materials"))
        {
            double roughness                = getOptional(m, "roughness", 0.0);
            double ior                      = getOptional(m, "ior", scene_ior);
            double transparency             = getOptional(m, "transparency", 0.0);
            bool perfect_mirror             = getOptional(m, "perfect_mirror", false);
            glm::dvec3 reflectance          = getOptional(m, "reflectance", glm::dvec3(0.0));
            glm::dvec3 specular_reflectance = getOptional(m, "specular_reflectance", glm::dvec3(0.0));
            glm::dvec3 emittance            = getOptional(m, "emittance", glm::dvec3(0.0));

            auto mat = std::make_shared<Material>(reflectance, specular_reflectance, emittance, roughness, ior, transparency, perfect_mirror, scene_ior);
            materials.insert({ m.at("name"), mat });
        }

        int i = 0;
        std::vector<std::vector<glm::dvec3>> vertices; // [set][vertices]
        for (const auto &s : j.at("vertices"))
        {
            vertices.push_back(std::vector<glm::dvec3>());
            for (const auto &v : s)
            {
                vertices[i].push_back(j2v(v));
            }
            i++;
        }

        std::vector<std::shared_ptr<Surface::Base>> surfaces;
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
                const auto& v = vertices.at(s.at("set"));

                bool is_emissive = material->emittance.r > C::EPSILON || material->emittance.g > C::EPSILON || material->emittance.b > C::EPSILON;
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
                surfaces.push_back(std::make_shared<Surface::Triangle>(j2v(v.at(0)), j2v(v.at(1)), j2v(v.at(2)), material));
            }
            else if (type == "sphere")
            {	
                // TODO: Remove radius scale and change radii of spheres in scene file
                double radius = s.at("radius");
                radius *= 0.99;
                surfaces.push_back(std::make_shared<Surface::Sphere>(j2v(s.at("origin")), radius, material));
            }
        }
        scene_file.close();

        int threads = getOptional(j, "num_render_threads", -1);
        std::shared_ptr<Scene> scene = std::make_shared<Scene>(surfaces, j.at("sqrtspp"), j.at("savename"), threads, scene_ior);

        std::shared_ptr<PhotonMap> photon_map;
        if (j.find("photon_map") != j.end())
        {
            char a;
            std::cout << "Use photon mapping? (y/n) ";
            while (std::cin >> a)
            {
                if (a == 'y' || a == 'Y' || a == 'n' || a == 'N') break;
                std::cout << "Answer with the letters y or n: ";
            }
            std::cout << std::endl;
            if(a == 'y' || a == 'Y')
            { 
                const auto& pm = j.at("photon_map");
                photon_map = std::make_shared<PhotonMap>(
                    scene, pm.at("emissions"), pm.at("max_photons_per_octree_leaf"),
                    pm.at("caustic_factor"), pm.at("radius"), pm.at("caustic_radius")
                );
            }
        }
        return SceneRenderer(camera, scene, photon_map);
    }
};