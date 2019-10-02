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

		Camera camera(j2v(c["eye"]), j2v(c["forward"]), j2v(c["up"]), c["focal_length"], c["sensor_width"], c["image_size"][0], c["image_size"][1]);

		if (c.find("lookat") != c.end())
		{
			camera.lookAt(j2v(c["lookat"]));
		}

		std::unordered_map<std::string, Material> materials;
		for (const auto& m : j["materials"])
		{
			materials[m["name"]] = Material(j2v(m["reflectance"]), j2v(m["emittance"]), m["roughness"], m["specular"]);
		}

		std::vector<glm::dvec3> vertices;
		for (const auto& v : j["vertices"])
		{
			vertices.push_back(j2v(v));
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
				for (const auto& t : s["triangles"])
				{
					const auto& v = vertices;
					surfaces.push_back(std::make_shared<Surface::Triangle>(v[t[0]], v[t[1]], v[t[2]], materials[material]));
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

		camera.sampleImage(scene.sqrtspp, scene);
		camera.saveImage(scene.savename);

		return scene;
	}
};