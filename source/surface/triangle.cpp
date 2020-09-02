#include "surface.hpp"

#include <glm/gtx/transform.hpp>

#include "../common/constants.hpp"

Surface::Triangle::Triangle(const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2, std::shared_ptr<Material> material)
    : Base(material), v0(v0), v1(v1), v2(v2), E1(v1 - v0), E2(v2 - v0), normal_(glm::normalize(glm::cross(E1, E2))), N(nullptr)
{
    computeArea();
    computeBoundingBox();
}

Surface::Triangle::Triangle(const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2,
                            const glm::dvec3& n0, const glm::dvec3& n1, const glm::dvec3& n2, std::shared_ptr<Material> material)
    : Base(material), v0(v0), v1(v1), v2(v2), E1(v1 - v0), E2(v2 - v0), normal_(glm::normalize(glm::cross(E1, E2))), 
      N(std::make_unique<glm::dmat3>(glm::normalize(n0), glm::normalize(n1), glm::normalize(n2)))
{
    computeArea();
    computeBoundingBox();
}

bool Surface::Triangle::intersect(const Ray& ray, Intersection& intersection) const
{
    glm::dvec3 P = glm::cross(ray.direction, E2);
    double determinant = glm::dot(P, E1);
    if (std::abs(determinant) < C::EPSILON) // Ray parallel to triangle. 
    {
        return false;
    }

    double inv_determinant = 1.0 / determinant;

    glm::dvec3 T = ray.start - v0;
    double u = glm::dot(P, T) * inv_determinant;
    if (u > 1.0 || u < 0.0)
    {
        return false;
    }

    glm::dvec3 Q = glm::cross(T, E1);
    double v = glm::dot(Q, ray.direction) * inv_determinant;
    if (v > 1.0 || v < 0.0 || u + v > 1.0)
    {
        return false;
    }

    double t = glm::dot(Q, E2) * inv_determinant;
    if (t <= 0.0)
    {
        return false;
    }

    intersection = Intersection(t);

    if (N)
    {
        intersection.uv = { u, v };
        intersection.interpolate = true;
    }

    return true;
}

void Surface::Triangle::transform(const Transform &T)
{
    v0 = T.matrix * glm::dvec4(v0, 1.0);
    v1 = T.matrix * glm::dvec4(v1, 1.0);
    v2 = T.matrix * glm::dvec4(v2, 1.0);

    E1 = v1 - v0;
    E2 = v2 - v0;
    normal_ = glm::normalize(glm::cross(E1, E2));

    if (N)
    {
        auto &vn = *N;
        vn[0] = T.rotation_matrix * glm::dvec4(glm::normalize(vn[0] / T.scale), 1.0);
        vn[1] = T.rotation_matrix * glm::dvec4(glm::normalize(vn[1] / T.scale), 1.0);
        vn[2] = T.rotation_matrix * glm::dvec4(glm::normalize(vn[2] / T.scale), 1.0);
    }

    computeArea();
    computeBoundingBox();
}

glm::dvec3 Surface::Triangle::operator()(double u, double v) const
{
    double su = std::sqrt(u);
    return (1 - su) * v0 + (1 - v) * su * v1 + v * su * v2;
}

glm::dvec3 Surface::Triangle::normal(const glm::dvec3& pos) const
{
    return normal_;
}

glm::dvec3 Surface::Triangle::normal() const
{
    return normal_;
}

glm::dvec3 Surface::Triangle::interpolatedNormal(const glm::dvec2& uv) const
{
    const auto &vn = *N;
    return glm::normalize((1.0 - uv.x - uv.y) * vn[0] + uv.x * vn[1] + uv.y * vn[2]);
}

void Surface::Triangle::computeBoundingBox()
{
    BB_ = BoundingBox();
    for (const auto &v : {v0, v1, v2})
    {
        BB_.merge(v);
    }
}

void Surface::Triangle::computeArea()
{
    area_ = glm::length(glm::cross(E1, E2)) / 2.0;
}