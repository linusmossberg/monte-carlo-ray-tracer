#pragma once

#include <vector>

#include <nanoflann.hpp>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include "Util.hpp"
#include "Scene.hpp"
//#include "Octree.hpp"
#include "Octree.hpp"

struct Photon : public OctreeData
{
public:
    Photon(const glm::vec3& flux, const glm::vec3& position, const glm::vec3& direction)
        : flux(flux), position(position), direction(direction) { }

    // Shadow photon constructor
    Photon(const glm::vec3& position)
        : flux(0.0f), position(position), direction(0.0f) { }

    virtual glm::vec3 pos() const
    {
        return position;
    }

    glm::vec3 flux, position, direction;
};

class PhotonMap
{
public:
    PhotonMap(Scene& s, uint16_t max_points)
        : global(glm::vec3(5,0,0), glm::vec3(8, 5, 6) + glm::vec3(0.01f), max_points)
    {
        scene = std::make_shared<Scene>(s);
        for (const auto& light : scene->emissives)
        {
            glm::dvec3 flux = light->material->emittance * light->area() / 2000000.0;
            for (int i = 0; i < 2000000; i++)
            {
                glm::dvec3 pos = (*light)(Random::range(0, 1), Random::range(0, 1));
                glm::dvec3 normal = light->normal(pos);
                glm::dvec3 dir = CoordinateSystem::localToGlobalUnitVector(Random::UniformHemiSample(), normal);

                pos += normal * 1e-7;

                emitPhoton(Ray(pos, pos + dir, scene->ior), flux);
            }
        }
    }

    void emitPhoton(const Ray& ray, const glm::dvec3& flux, size_t ray_depth = 0)
    {
        Intersection intersect = scene->intersect(ray, true);

        if (!intersect)
            return;

        Ray new_ray(intersect.position);

        glm::dvec3 BRDF;

        double n1 = ray.medium_ior;
        double n2 = abs(ray.medium_ior - scene->ior) < 1e-7 ? intersect.material->ior : scene->ior;

        if (intersect.material->perfect_mirror || Material::Fresnel(n1, n2, intersect.normal, -ray.direction) > Random::range(0, 1))
        {
            /* SPECULAR REFLECTION */
            if(ray_depth == 0) 
                createShadowPhotons(Ray(intersect.position - intersect.normal * 1e-7, intersect.position + ray.direction));

            BRDF = intersect.material->SpecularBRDF();
            new_ray.reflectSpecular(ray.direction, intersect.normal, n1);
        }
        else
        {
            if (intersect.material->transparency > Random::range(0, 1))
            {
                /* SPECULAR REFRACTION */
                // TODO: Maybe create shadow photons if reflection
                BRDF = intersect.material->SpecularBRDF();
                new_ray.refractSpecular(ray.direction, intersect.normal, n1, n2);
            }
            else
            {
                /* DIFFUSE REFLECTION */
                global.insert(std::make_unique<Photon>(flux, intersect.position, ray.direction));
                global_vec.push_back(Photon(flux, intersect.position, ray.direction));

                if (ray_depth == 0) 
                    createShadowPhotons(Ray(intersect.position - intersect.normal * 1e-7, intersect.position + ray.direction));

                double terminate = 1.0 - intersect.material->reflect_probability;
                if (terminate > Random::range(0, 1))
                    return;

                CoordinateSystem cs(intersect.normal);
                BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction)) / (1.0 - terminate);
                new_ray.reflectDiffuse(cs, n1);
            }
        }
        emitPhoton(new_ray, flux * BRDF, ray_depth + 1);
    }

    void createShadowPhotons(const Ray& ray)
    {
        Intersection intersect = scene->intersect(ray, true);

        if (!intersect)
            return;

        // TODO: Might need to evaluate fresnel and transparency to probabilistically determine if shadow photon should be created
        if ((abs(intersect.material->transparency - 1.0) > 1e-7) && !intersect.material->perfect_mirror)
        {
            global_vec.push_back(Photon(intersect.position));
            global.insert(std::make_unique<Photon>(intersect.position));
        }

        glm::dvec3 pos(intersect.position - intersect.normal * 1e-7);
        createShadowPhotons(Ray(pos, pos + ray.direction));
    }

    Octree global;

    std::vector<Photon> global_vec;

    std::shared_ptr<Scene> scene;
};