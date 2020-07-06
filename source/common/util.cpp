#include <fstream>
#include <iostream>

#include "util.hpp"
#include "format.hpp"

void glm::from_json(const nlohmann::json &j, dvec3 &v)
{
    if (j.type() == nlohmann::json::value_t::array)
        for (int i = 0; i < 3; i++) j.at(i).get_to(v[i]);
    else
        for (int i = 0; i < 3; i++) j.get_to(v[i]);
}

void waitForInput()
{
    std::cout << std::endl << "Press enter to exit." << std::flush;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    char c;
    while (std::cin.get(c) && c != '\n') {}
}

void Log(const std::string& message)
{
    std::ofstream log("log.txt", std::ios::app);
    std::string temp = message;
    temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
    log << "[" << Format::date(std::chrono::system_clock::now()) << "] " << temp << std::endl;
}