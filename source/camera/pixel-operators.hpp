#pragma once

#include <vector>

#include <glm/vec3.hpp>

glm::dvec3 filmic(glm::dvec3 in);

glm::dvec3 gammaCorrect(glm::dvec3 in);

std::vector<uint8_t> truncate(glm::dvec3 in);