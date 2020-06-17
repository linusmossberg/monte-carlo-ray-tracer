#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <nlohmann/json.hpp>

#include "../ray/ray.hpp"
#include "../ray/intersection.hpp"
#include "../material/material.hpp"
#include "../common/bounding-box.hpp"

namespace Surface
{
    class Base
    {
    public:
        Base(std::shared_ptr<Material> material)
            : material(material), area_(0) { };

        virtual ~Base() { }

        virtual bool intersect(const Ray& ray, Intersection& intersection) const = 0;
        virtual glm::dvec3 operator()(double u, double v) const = 0; // point on surface
        virtual glm::dvec3 normal(const glm::dvec3& pos) const = 0;

        BoundingBox boundingBox() const
        {
            return BB;
        }

        virtual glm::dvec3 midPoint() const = 0;

        double area() const
        {
            return area_;
        }

        std::shared_ptr<Material> material;

        BoundingBox BB;

    protected:
        virtual void computeArea() = 0;
        virtual void computeBoundingBox() = 0;
        double area_;
    };

    class Sphere : public Base
    {
    public:
        Sphere(const glm::dvec3& origin, double radius, std::shared_ptr<Material> material);

        virtual bool intersect(const Ray& ray, Intersection& intersection) const;
        virtual glm::dvec3 operator()(double u, double v) const;
        virtual glm::dvec3 normal(const glm::dvec3& pos) const;
        virtual glm::dvec3 midPoint() const;

    protected:
        virtual void computeArea();
        virtual void computeBoundingBox();

    private:
        glm::dvec3 origin;
        double radius;
    };

    class Triangle : public Base
    {
    public:
        Triangle(const glm::dvec3& v0, const glm::dvec3& v1, const glm::dvec3& v2, std::shared_ptr<Material> material);

        virtual bool intersect(const Ray& ray, Intersection& intersection) const;
        virtual glm::dvec3 operator()(double u, double v) const;
        virtual glm::dvec3 normal(const glm::dvec3& pos) const;
        virtual glm::dvec3 midPoint() const;

    protected:
        virtual void computeArea();
        virtual void computeBoundingBox();

    private:
        glm::dvec3 v0, v1, v2;

        // Pre-computed edges and normal
        glm::dvec3 E1, E2, normal_;
    };

    class Quadric : public Base
    {
    public:
        Quadric(const nlohmann::json &j, std::shared_ptr<Material> material);

        virtual bool intersect(const Ray& ray, Intersection& intersection) const;
        virtual glm::dvec3 operator()(double u, double v) const;
        virtual glm::dvec3 normal(const glm::dvec3& pos) const;
        virtual glm::dvec3 midPoint() const;

    protected:
        virtual void computeArea();
        virtual void computeBoundingBox() { }

    private:
        glm::dmat4x4 Q; // Quadric matrix
        glm::dmat4x3 G; // Gradient matrix
    };
}
