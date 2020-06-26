#pragma once

#include <vector>

#include <glm/vec3.hpp>

glm::dvec3 filmicHable(const glm::dvec3 &in);

glm::dvec3 filmicACES(const glm::dvec3 &in);

glm::dvec3 gammaCorrect(const glm::dvec3 &in);

std::vector<uint8_t> truncate(const glm::dvec3 &in);