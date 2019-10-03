#include <iostream>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <filesystem>

#include <fstream>
#include <sstream>

#include <glm/vec3.hpp>

#include "SceneParser.h"
#include "Random.h"
#include "Util.h"
#include "Scene.h"
#include "Camera.h"

int main()
{
	Random::seed(std::random_device{}());

	std::filesystem::path path(std::filesystem::current_path().string() + "\\scenes");
	std::cout << "Scene directory:" << std::endl << path.string() << std::endl << std::endl;

	std::vector<std::pair<std::filesystem::path, int>> options;

	try
	{
		options = SceneParser::availible(path);
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
		return -1;
	}

	if (options.size() == 0)
	{
		std::cout << "No scenes found." << std::endl;
		return -1;
	}

	size_t scene_option = printSceneOptionTable(options);

	std::string name = options[scene_option].first.filename().string();
	name.erase(name.find("."), name.length());
	std::cout << std::endl << "Scene " << name << " with camera " << options[scene_option].second << " selected." << std::endl << std::endl;
	
	try
	{
		Scene scene = SceneParser::parseScene(options[scene_option].first, options[scene_option].second);
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
		return -1;
	}
	
	return 0;
}