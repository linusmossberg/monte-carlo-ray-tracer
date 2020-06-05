#include "util.hpp"

void glm::from_json(const nlohmann::json &j, dvec3 &v)
{
    for (int i = 0; i < 3; i++) j.at(i).get_to(v[i]);
}