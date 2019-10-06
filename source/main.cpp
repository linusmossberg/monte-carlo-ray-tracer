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
		waitForInput();
		return -1;
	}

	if (options.size() == 0)
	{
		std::cout << "No scenes found." << std::endl;
		waitForInput();
		return -1;
	}

	size_t scene_option = printSceneOptionTable(options);

	std::string file = options[scene_option].first.filename().string();
	file.erase(file.find("."), file.length());
	std::cout << "Scene file " << file << " with camera " << options[scene_option].second << " selected." << std::endl << std::endl;
	
	try
	{
		Scene scene = SceneParser::parseScene(options[scene_option].first, options[scene_option].second);
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
		waitForInput();
		return -1;
	}

	waitForInput();
	
	return 0;
}