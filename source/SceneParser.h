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
	return glm::dvec3(j[0], j[1], j[2]);
}

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
			for (const auto& cameras : j["cameras"])
			{
				options.push_back(std::make_pair(file.path(), i));
				i++;
			}
			scene_file.close();
		}
		return options;
	}

	static Scene parseScene(const std::filesystem::path path, int camera_idx)
	{
		std::ifstream scene_file(path);
		nlohmann::json j;
		scene_file >> j;

		const auto& c = j["cameras"][camera_idx];

		std::unique_ptr<Camera> camera;
		if (c.find("look_at") != c.end())
		{
			camera = std::make_unique<Camera>(
				j2v(c["eye"]), j2v(c["look_at"]), 
				c["focal_length"], c["sensor_width"], 
				c["width"], c["height"]
			);
		}
		else
		{
			camera = std::make_unique<Camera>(
				j2v(c["eye"]), j2v(c["forward"]), j2v(c["up"]), 
				c["focal_length"], c["sensor_width"], 
				c["width"], c["height"]
			);
		}

		std::unordered_map<std::string, Material> materials;
		for (const auto& m : j["materials"])
		{
			materials[m["name"]] = Material(j2v(m["reflectance"]), j2v(m["emittance"]), m["roughness"], m["specular"]);
		}

		int i = 0;
		std::vector<std::vector<glm::dvec3>> vertices; // [set][vertices]
		for (const auto &s : j["vertices"])
		{
			vertices.push_back(std::vector<glm::dvec3>());
			for (const auto &v : s)
			{
				vertices[i].push_back(j2v(v));
			}
			i++;
		}

		std::vector<std::shared_ptr<Surface::Base>> surfaces;
		for (const auto& s : j["surfaces"])
		{
			std::string material = "default";
			if (s.find("material") != s.end())
			{
				material = s["material"];
			}

			std::string type = s["type"];
			if (type == "object")
			{
				const auto& v = vertices[s["set"]];
				double total_area = 0.0;
				for (const auto& t : s["triangles"])
				{
					total_area += Surface::Triangle(v[t[0]], v[t[1]], v[t[2]]).area();
				}
				
				for (const auto& t : s["triangles"])
				{
					// Entire object emits the flux of assigned material emittance in scene file.
					// The flux of the material therefore needs to be distributed amongst all object triangles.
					Material mat = materials[material];
					double area = Surface::Triangle(v[t[0]], v[t[1]], v[t[2]]).area();
					mat.emittance *= area / total_area;
					surfaces.push_back(std::make_shared<Surface::Triangle>(v[t[0]], v[t[1]], v[t[2]], mat));
				}
			}
			else if (type == "triangle")
			{
				const auto& v = s["vertices"];
				surfaces.push_back(std::make_shared<Surface::Triangle>(j2v(v[0]), j2v(v[1]), j2v(v[2]), materials[material]));
			}
			else if (type == "sphere")
			{	
				surfaces.push_back(std::make_shared<Surface::Sphere>(j2v(s["origin"]), s["radius"], materials[material]));
			}
		}
		scene_file.close();

		Scene scene(Scene(surfaces, j["settings"]["sqrtspp"], j["settings"]["savename"]));

		camera->sampleImage(scene.sqrtspp, scene);
		camera->saveImage(scene.savename);

		return scene;
	}
};