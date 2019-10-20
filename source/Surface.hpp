#pragma once

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Ray.hpp"
#include "Material.hpp"
#include "Constants.hpp"
#include "Util.hpp"

namespace Surface
{
    class Base
    {
    public:
        Base() 
            : material(std::make_shared<Material>()), area_(0) { };

        Base(const Material& material)
            : material(std::make_shared<Material>(material)), area_(0) { };

        virtual ~Base() {};

        virtual bool intersect(const Ray& ray, Intersection& intersection) const = 0;

        virtual glm::dvec3 operator()(double u, double v) const = 0; // point on surface

        virtual glm::dvec3 normal(const glm::dvec3& pos) const = 0;

        virtual BoundingBox boundingBox() const = 0;

        double area() const
        {
            return area_;
        }

        std::shared_ptr<Material> material;

    protected:
        virtual void computeArea() = 0;
        double area_;
    };

    class Sphere : public Base
    {
    public:
        Sphere(const glm::dvec3& origin, double radius)
            : origin(origin), radius(radius) { computeArea(); }

        Sphere(const glm::dvec3& origin, double radius, const Material& material)
            : Base(material), origin(origin), radius(radius) { computeArea(); }

        ~Sphere() {};

        virtual bool intersect(const Ray& ray, Intersection& intersection) const;

        virtual glm::dvec3 operator()(double u, double v) const
        {
            double z = 1.0 - 2.0 * u;
            double r = std::sqrt(1.0 - pow2(z));
            double phi = C::TWO_PI * v;

            return origin + radius * glm::dvec3(r * cos(phi), r * sin(phi), z);
        }

        virtual glm::dvec3 normal(const glm::dvec3& pos) const
        {
            return (pos - origin) / radius;
        }

        virtual BoundingBox boundingBox() const
        {
            return BoundingBox(
                glm::dvec3(origin - glm::dvec3(radius)), 
                glm::dvec3(origin + glm::dvec3(radius))
            );
        }

    protected:
        virtual void computeArea()
        {
            area_ = 2.0 * C::TWO_PI * pow2(radius);
        }

    private:
        glm::dvec3 origin;
        double radius;
    };

    class Triangle : public Base
    {
    public:
        Triangle(const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2)
            : v0(v0), v1(v1), v2(v2), E1(v1 - v0), E2(v2 - v0), normal_(glm::normalize(glm::cross(E1, E2))) { computeArea(); }

        Triangle(const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2, const Material& material)
            : Base(material), v0(v0), v1(v1), v2(v2), E1(v1 - v0), E2(v2 - v0), normal_(glm::normalize(glm::cross(E1, E2))) { computeArea(); }

        ~Triangle() {};

        virtual bool intersect(const Ray& ray, Intersection& intersection) const;

        virtual glm::dvec3 operator()(double u, double v) const
        {
            double su = std::sqrt(u);
            return (1 - su) * v0 + (1 - v) * su * v1 + v * su * v2;
        }

        virtual glm::dvec3 normal(const glm::dvec3& pos) const
        {
            return normal_;
        }

        virtual BoundingBox boundingBox() const
        {
            BoundingBox bb;
            for (uint8_t c = 0; c < 3; c++)
            {
                bb.min[c] = std::min(v0[c], std::min(v1[c], v2[c]));
                bb.max[c] = std::max(v0[c], std::max(v1[c], v2[c]));
            }
            return bb;
        }

    protected:
        virtual void computeArea()
        {
            area_ = glm::length(glm::cross(E1, E2)) / 2.0;
        }

    private:
        glm::dvec3 v0, v1, v2;

        // Pre-computed edges and normal
        glm::dvec3 E1, E2, normal_;
    };
}
