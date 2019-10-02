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

	size_t max_opt = 13, max_fil = 0, max_cam = 7;
	for (const auto& o : options)
	{
		std::string file = o.first.filename().string();
		file.erase(file.find("."), file.length());

		if (file.size() > max_fil)
		{
			max_fil = file.size();
		}
	}
	max_fil++;

	std::cout << " " << std::string(max_opt + max_fil + max_cam + 5, '_') << std::endl;

	std::cout << "| " << std::left << std::setw(max_opt) << "Scene option";
	std::cout << "| " << std::left << std::setw(max_fil) << "File";
	std::cout << "| " << std::left << std::setw(max_cam) << "Camera" ;
	std::cout << "| " << std::endl;

	std::cout << "|" << std::string(max_opt + 1, '_') << '|' << std::string(max_fil + 1, '_') << '|' << std::string(max_cam + 1, '_') << '|' << std::endl;

	for (int i = 0; i < options.size(); i++)
	{
		std::string file = options[i].first.filename().string();
		file.erase(file.find("."), file.length());

		std::cout << "| " << std::left << std::setw(max_opt) << i;
		std::cout << "| " << std::left << std::setw(max_fil) << file;
		std::cout << "| " << std::left << std::setw(max_cam) << options[i].second;
		std::cout << "| " << std::endl;

		std::cout << "|" << std::string(max_opt + 1, '_') << '|' << std::string(max_fil + 1, '_') << '|' << std::string(max_cam + 1, '_') << '|' << std::endl;
	}

	int scene_option;
	std::cout << std::endl << "Select scene option: ";
	//while (std::cin >> scene_option)
	//{
	//	if (scene_option < 0 || scene_option >= options.size())
	//		std::cout << "Invalid scene number, try again: ";
	//	else
	//		break;
	//}
	scene_option = 0;

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