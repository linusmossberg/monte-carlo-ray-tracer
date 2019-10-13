#pragma once

#include <vector>
#include <memory>

#include "Surface.hpp"
#include "Ray.hpp"
#include "Random.hpp"

class Scene
{
public:
    Scene(std::vector<std::shared_ptr<Surface::Base>> surfaces, size_t sqrtspp, const std::string &savename, double ior)
        : surfaces(surfaces), sqrtspp(sqrtspp), savename(savename), ior(ior)
    {
        generateEmissives();
    }

    Intersection intersect(const Ray& ray, bool align_normal = false, double min_distance = -1)
    {
        bool use_stop_condition = min_distance > 0;

        Intersection intersect;
        for (const auto& surface : surfaces)
        {
            Intersection t_intersect;
            if (surface->intersect(ray, t_intersect))
            {
                if (use_stop_condition && t_intersect.t < min_distance)
                    return Intersection();

                if (t_intersect.t < intersect.t)
                    intersect = t_intersect;
            }
        }
        if (align_normal && glm::dot(ray.direction, intersect.normal) > 0)
        {
            intersect.normal = -intersect.normal;
        }
        return intersect;
    }

    glm::dvec3 sampleLights(const Intersection &intersection)
    {
        // Pick one light source and scale with 1/probability of picking light source
        if (emissives.size())
        {
            const auto &light = emissives[Random::uirange(0, emissives.size() - 1)];

            glm::dvec3 light_pos = light->operator()(Random::range(0,1), Random::range(0,1));
            Ray shadow_ray(intersection.position + intersection.normal * 1e-7, light_pos);

            double cos_theta = glm::dot(shadow_ray.direction, intersection.normal);

            if (cos_theta <= 0)
                return glm::dvec3(0.0);

            double min_distance = glm::distance(light_pos, intersection.position) - 1e-7;
            
            Intersection shadow_intersection = intersect(shadow_ray, false, min_distance);

            if (shadow_intersection)
            {
                double cos_light_theta = glm::dot(shadow_intersection.normal, -shadow_ray.direction);

                if (cos_light_theta <= 0 || glm::distance(shadow_intersection.position, light_pos) > 1e-7)
                    return glm::dvec3(0.0);

                double t = light->area() * cos_light_theta / pow2(shadow_intersection.t);
                return (light->material->emittance * t * cos_theta * static_cast<double>(emissives.size())) / M_PI; // * pi to make the BRDF*pi applicable later
            }
        }
        return glm::dvec3(0.0);
    }

    void generateEmissives()
    {
        for (const auto &surface : surfaces)
        {
            if (glm::length(surface->material->emittance) >= 1e-7)
            {
                surface->material->emittance /= surface->area(); // flux to radiosity
                emissives.push_back(surface);
            }
        }
    }

    glm::dvec3 skyColor(const Ray& ray) const
    {
        double f = (glm::dot(glm::dvec3(0.0, 1.0, 0.0), ray.direction) * 0.7 + 1.0) / 2.0;
        if (f < 0.5)
        {
            return glm::dvec3(0.5); // ground color
        }
        return glm::dvec3(1.0 - f, 0.5, f);
    }

    std::vector<std::shared_ptr<Surface::Base>> surfaces;
    std::vector<std::shared_ptr<Surface::Base>> emissives; // subset of surfaces
    size_t sqrtspp;
    std::string savename;
    double ior;
};