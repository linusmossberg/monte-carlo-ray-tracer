#pragma once

#include <fstream>
#include <filesystem>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Scene.h"
#include "Camera.h"
#include "Material.h"

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

struct CameraScenePair
{
	CameraScenePair(Camera &c, Scene &s) : camera(c), scene(s) { }

	Camera camera;
	Scene scene;
};

class SceneParser
{
public:
	static std::vector<std::pair<std::filesystem::path, int>> availible(std::filesystem::path path)
	{
		std::vector<std::pair<std::filesystem::path, int>> options; // <scene, camera>
		for (const auto& file : std::filesystem::directory_iterator(path))
		{
			if (!file.path().has_extension() || file.path().extension() != ".json")
				continue;

			std::ifstream scene_file(file.path());
			nlohmann::json j;
			scene_file >> j;
			int i = 0;
			for (const auto& cameras : j.at("cameras"))
			{
				options.push_back(std::make_pair(file.path(), i));
				i++;
			}
			scene_file.close();
		}
		return options;
	}

	static CameraScenePair parseScene(const std::filesystem::path path, int camera_idx)
	{
		std::ifstream scene_file(path);
		nlohmann::json j;
		scene_file >> j;

		const auto& c = j.at("cameras").at(camera_idx);

		std::unique_ptr<Camera> camera;
		if (c.find("look_at") != c.end())
		{
			camera = std::make_unique<Camera>(
				j2v(c.at("eye")), j2v(c.at("look_at")), 
				c.at("focal_length"), c.at("sensor_width"), 
				c.at("width"), c.at("height")
			);
		}
		else
		{
			camera = std::make_unique<Camera>(
				j2v(c.at("eye")), j2v(c.at("forward")), j2v(c.at("up")), 
				c.at("focal_length"), c.at("sensor_width"), 
				c.at("width"), c.at("height")
			);
		}
		camera->setNaive(getOptional(j, "naive", false));

		std::unordered_map<std::string, Material> materials;
		for (const auto& m : j.at("materials"))
		{
			double roughness                = getOptional(m, "roughness", 0.0);
			double ior                      = getOptional(m, "ior", 1.0);
			double transparency             = getOptional(m, "transparency", 0.0);
			bool perfect_specular           = getOptional(m, "perfect_specular", false);
			glm::dvec3 reflectance          = getOptional(m, "reflectance", glm::dvec3(0.0));
			glm::dvec3 specular_reflectance = getOptional(m, "specular_reflectance", glm::dvec3(0.0));
			glm::dvec3 emittance            = getOptional(m, "emittance", glm::dvec3(0.0));

			materials[m.at("name")] = Material(reflectance, specular_reflectance, emittance, roughness, ior, transparency, perfect_specular);
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
			std::string material = "default";
			if (s.find("material") != s.end())
			{
				material = s.at("material");
			}

			std::string type = s.at("type");
			if (type == "object")
			{
				const auto& v = vertices.at(s.at("set"));
				double total_area = 0.0;
				for (const auto& t : s.at("triangles"))
				{
					total_area += Surface::Triangle(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2))).area();
				}
				
				for (const auto& t : s.at("triangles"))
				{
					// Entire object emits the flux of assigned material emittance in scene file.
					// The flux of the material therefore needs to be distributed amongst all object triangles.
					Material mat = materials.at(material);
					double area = Surface::Triangle(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2))).area();
					mat.emittance *= area / total_area;
					surfaces.push_back(std::make_shared<Surface::Triangle>(v.at(t.at(0)), v.at(t.at(1)), v.at(t.at(2)), mat));
				}
			}
			else if (type == "triangle")
			{
				const auto& v = s.at("vertices");
				surfaces.push_back(std::make_shared<Surface::Triangle>(j2v(v.at(0)), j2v(v.at(1)), j2v(v.at(2)), materials.at(material)));
			}
			else if (type == "sphere")
			{	
				// TODO: Remove radius scale and change radii of spheres in scene file
				double radius = s.at("radius");
				radius *= 0.99;
				surfaces.push_back(std::make_shared<Surface::Sphere>(j2v(s.at("origin")), radius, materials.at(material)));
			}
		}
		scene_file.close();

		Scene scene(surfaces, j.at("sqrtspp"), j.at("savename"), j.at("ior"));

		return CameraScenePair(*camera, scene);
	}
};