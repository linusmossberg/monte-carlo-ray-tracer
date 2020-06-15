#include "surface.hpp"

#include "../common/util.hpp"
#include "../common/constants.hpp"

Surface::Sphere::Sphere(const glm::dvec3& origin, double radius, std::shared_ptr<Material> material)
    : Base(material), origin(origin), radius(radius) 
{
    computeArea();
    computeBoundingBox();
}

bool Surface::Sphere::intersect(const Ray& ray, Intersection& intersection) const
{
    glm::dvec3 so = ray.start - origin;
    double b = glm::dot(ray.direction, so);
    double c = glm::dot(so, so) - pow2(radius);

    double discriminant = pow2(b) - c;
    if (discriminant < 0)
    {
        return false;
    }

    double v = std::sqrt(discriminant);
    double t = -b - v;
    if (t < 0)
    {
        t = v - b;
        if (t < 0)
        {
            return false;
        }
    }

    glm::dvec3 pos = ray(t);
    intersection = Intersection(pos, normal(pos), t, material);

    return true;
}

glm::dvec3 Surface::Sphere::operator()(double u, double v) const
{
    double z = 1.0 - 2.0 * u;
    double r = std::sqrt(1.0 - pow2(z));
    double phi = C::TWO_PI * v;

    return origin + radius * glm::dvec3(r * std::cos(phi), r * std::sin(phi), z);
}

glm::dvec3 Surface::Sphere::normal(const glm::dvec3& pos) const
{
    return (pos - origin) / radius;
}

glm::dvec3 Surface::Sphere::midPoint() const
{
    return origin;
}

void Surface::Sphere::computeArea()
{
    area_ = 2.0 * C::TWO_PI * pow2(radius);
}

void Surface::Sphere::computeBoundingBox()
{
    BB = BoundingBox(
        glm::dvec3(origin - glm::dvec3(radius)),
        glm::dvec3(origin + glm::dvec3(radius))
    );

    BB.computeProperties();
}