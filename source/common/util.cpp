#include <fstream>
#include <iostream>

#include <glm/gtx/transform.hpp>

#include "util.hpp"
#include "format.hpp"

void glm::from_json(const nlohmann::json &j, dvec3 &v)
{
    if (j.type() == nlohmann::json::value_t::array)
        for (int i = 0; i < 3; i++) j.at(i).get_to(v[i]);
    else
        for (int i = 0; i < 3; i++) j.get_to(v[i]);
}

Transform::Transform(glm::dvec3 position, glm::dvec3 scale, glm::dvec3 rotation)
    : position(position), scale(scale), rotation(rotation)
{
    rotation_matrix = glm::rotate(rotation.z, glm::dvec3(0.0, 0.0, 1.0)) *
                      glm::rotate(rotation.y, glm::dvec3(0.0, 1.0, 0.0)) *
                      glm::rotate(rotation.x, glm::dvec3(1.0, 0.0, 0.0));

    matrix = glm::translate(glm::dmat4(1.0), position) *
             rotation_matrix *
             glm::scale(glm::dmat4(1.0), scale);
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