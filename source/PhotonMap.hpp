#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vector_relational.hpp>

#include "Util.hpp"
#include "Scene.hpp"
#include "Octree.hpp"
#include "MultiThreading.hpp"
#include "Constants.hpp"

struct Photon : public OctreeData
{
public:
    Photon(const glm::dvec3& flux, const glm::dvec3& position, const glm::dvec3& direction)
        : flux(flux), position(position), direction(direction) { }

    virtual glm::dvec3 pos() const
    {
        return position;
    }

    glm::dvec3 flux, position, direction;
};

// Separate shadow photon type to reduce memory usage and improve shadow photon search performance.
struct ShadowPhoton : public OctreeData
{
public:
    ShadowPhoton(const glm::dvec3& position)
        : position(position) { }

    virtual glm::dvec3 pos() const
    {
        return position;
    }

    glm::dvec3 position;
};

class PhotonMap
{
public:
    PhotonMap(Scene& s, size_t photon_emissions, uint16_t max_node_data)
        : direct_map(glm::dvec3(5,0,0), glm::dvec3(8, 5, 6) + glm::dvec3(0.01f), max_node_data),
          indirect_map(glm::dvec3(5, 0, 0), glm::dvec3(8, 5, 6) + glm::dvec3(0.01f), max_node_data),
          caustic_map(glm::dvec3(5, 0, 0), glm::dvec3(8, 5, 6) + glm::dvec3(0.01f), max_node_data),
          shadow_map(glm::dvec3(5, 0, 0), glm::dvec3(8, 5, 6) + glm::dvec3(0.01f), max_node_data)
          
    {
        scene = std::make_shared<Scene>(s);

        struct EmissionWork
        {
            EmissionWork() : light(), num_emissions(0), photon_flux(0.0) { }

            EmissionWork(std::shared_ptr<Surface::Base> light, size_t num_emissions, const glm::dvec3& photon_flux)
                : light(light), num_emissions(num_emissions), photon_flux(photon_flux) { }

            std::shared_ptr<Surface::Base> light;
            size_t num_emissions;
            glm::dvec3 photon_flux;
        };

        const size_t EPW = 100000;
        std::vector<EmissionWork> work_vec;

        double total_add_flux = 0.0;
        for (const auto& light : scene->emissives)
        {
            total_add_flux += glm::compAdd(light->material->emittance * light->area());
        }

        for (const auto& light : scene->emissives)
        {
            glm::dvec3 light_flux = light->material->emittance * light->area();
            double photon_emissions_share = glm::compAdd(light_flux) / total_add_flux;
            size_t num_light_emissions = static_cast<size_t>(photon_emissions * photon_emissions_share);
            glm::dvec3 photon_flux = light_flux / static_cast<double>(num_light_emissions);

            size_t count = 0;
            while (count != num_light_emissions)
            {
                size_t emissions = count + EPW > num_light_emissions ? num_light_emissions - count : EPW;
                work_vec.emplace_back(light, emissions, photon_flux);
                count += emissions;
            }
        }

        std::shuffle(work_vec.begin(), work_vec.end(), Random::getEngine());
        WorkQueue<EmissionWork> work_queue(work_vec);

        std::vector<std::unique_ptr<std::thread>> threads(std::thread::hardware_concurrency() - 2);

        direct_vecs.resize(threads.size());
        indirect_vecs.resize(threads.size());
        caustic_vecs.resize(threads.size());
        shadow_vecs.resize(threads.size());

        for (size_t thread = 0; thread < threads.size(); thread++)
        {
            threads[thread] = std::make_unique<std::thread>
            (
                [this, &work_queue, thread]()
                {
                    Random::seed(std::random_device{}()); // Each thread uses different seed.

                    EmissionWork work;
                    while (work_queue.getWork(work))
                    {
                        for (size_t i = 0; i < work.num_emissions; i++)
                        {
                            glm::dvec3 pos = (*work.light)(Random::range(0, 1), Random::range(0, 1));
                            glm::dvec3 normal = work.light->normal(pos);
                            glm::dvec3 dir = CoordinateSystem::localToGlobalUnitVector(Random::UniformHemiSample(), normal);

                            pos += normal * C::EPSILON;

                            emitPhoton(Ray(pos, pos + dir, scene->ior), work.photon_flux, thread);
                        }
                    }
                }
            );
        }

        std::thread print_thread([&work_queue]()
        {
            while (!work_queue.empty())
            {
                double progress = work_queue.progress();
                std::cout << std::string("\rPhoton map " + formatProgress(progress) + " constructed.");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        });
        print_thread.join();

        for (auto& thread : threads)
        {
            thread->join();
        }
        
        size_t num_direct_photons = 0;
        size_t num_indirect_photons = 0;
        size_t num_caustic_photons = 0;
        size_t num_shadow_photons = 0;

        for (size_t thread = 0; thread < threads.size(); thread++)
        {
            for (const auto& p : direct_vecs[thread]) direct_map.insert(p);
            num_direct_photons += direct_vecs[thread].size();
            direct_vecs[thread].clear();

            for (const auto& p : indirect_vecs[thread]) indirect_map.insert(p);
            num_indirect_photons += indirect_vecs[thread].size();
            indirect_vecs[thread].clear();

            for (const auto& p : caustic_vecs[thread]) caustic_map.insert(p);
            num_caustic_photons += caustic_vecs[thread].size();
            caustic_vecs[thread].clear();

            for (const auto& p : shadow_vecs[thread]) shadow_map.insert(p);
            num_shadow_photons += shadow_vecs[thread].size();
            shadow_vecs[thread].clear();
        }

        std::cout << '\r' << std::string(30, ' ') << '\r';
        std::cout << std::setw(18) << "Direct photons: "   << formatLargeNumber(num_direct_photons)   << std::endl
                  << std::setw(18) << "Indirect photons: " << formatLargeNumber(num_indirect_photons) << std::endl
                  << std::setw(18) << "Caustic photons: "  << formatLargeNumber(num_caustic_photons)  << std::endl
                  << std::setw(18) << "Shadow photons: "   << formatLargeNumber(num_shadow_photons)   << std::endl << std::endl;
    }

    void emitPhoton(const Ray& ray, const glm::dvec3& flux, size_t thread, size_t ray_depth = 0)
    {
        if (ray_depth == max_ray_depth)
        {
            Log("Max photon ray depth reached.");
            return;
        }

        Intersection intersect = scene->intersect(ray, true);

        if (!intersect) return;

        // Use russian roulette regardless of material. Otherwise call stack 
        // overflow (or bias if prevented) is guaranteed with some scenes.
        double terminate = ray_depth > 0 ? 1.0 - intersect.material->reflect_probability : 0.0;
        bool should_terminate = terminate > Random::range(0, 1);

        Ray new_ray(intersect.position);

        glm::dvec3 BRDF;

        double n1 = ray.medium_ior;
        double n2 = abs(ray.medium_ior - scene->ior) < C::EPSILON ? intersect.material->ior : scene->ior;

        if (intersect.material->perfect_mirror || Material::Fresnel(n1, n2, intersect.normal, -ray.direction) > Random::range(0, 1))
        {
            /* SPECULAR REFLECTION */
            if (ray_depth == 0 && Random::range(0, 1) < NON_CAUSTIC_REJECT)
                createShadowPhotons(Ray(intersect.position - intersect.normal * C::EPSILON, intersect.position + ray.direction), thread);

            if (should_terminate) return;

            BRDF = intersect.material->SpecularBRDF();
            new_ray.reflectSpecular(ray.direction, intersect.normal, n1);
        }
        else
        {
            if (intersect.material->transparency > Random::range(0, 1))
            {
                /* SPECULAR REFRACTION */
                if (should_terminate) return;

                BRDF = intersect.material->SpecularBRDF();

                new_ray.refractSpecular(ray.direction, intersect.normal, n1, n2);
               
                if (ray_depth == 0 && Random::range(0, 1) < NON_CAUSTIC_REJECT)
                {
                    createShadowPhotons(Ray(intersect.position - intersect.normal * C::EPSILON, intersect.position + ray.direction), thread);
                }
            }
            else
            {
                /* DIFFUSE REFLECTION */
                if (ray_depth == 0)
                {
                    if (Random::range(0, 1) < NON_CAUSTIC_REJECT)
                    {
                        direct_vecs[thread].emplace_back(flux / NON_CAUSTIC_REJECT, intersect.position, ray.direction);
                        createShadowPhotons(Ray(intersect.position - intersect.normal * C::EPSILON, intersect.position + ray.direction), thread);
                    }
                }
                else
                {
                    if (ray.specular)
                    {
                        caustic_vecs[thread].emplace_back(flux, intersect.position, ray.direction);
                    }
                    else
                    {
                        if (Random::range(0, 1) < NON_CAUSTIC_REJECT)
                        {
                            indirect_vecs[thread].emplace_back(flux / NON_CAUSTIC_REJECT, intersect.position, ray.direction);
                        }
                    }
                }

                if (should_terminate) return;

                CoordinateSystem cs(intersect.normal);
                new_ray.reflectDiffuse(cs, n1);
                BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction));
            }
        }
        emitPhoton(new_ray, flux * BRDF / (1.0 - terminate), thread, ray_depth + 1);
    }

    void createShadowPhotons(const Ray& ray, size_t thread)
    {
        Intersection intersect = scene->intersect(ray, true);

        if (!intersect) return;

        // This will create some unnecessary shadow photons, but evaluating transparency and IOR 
        // to see if there's a chance that this point could diffusely reflect might be more costly.
        if (!intersect.material->perfect_mirror)
        {
            shadow_vecs[thread].emplace_back(intersect.position);
        }

        glm::dvec3 pos(intersect.position - intersect.normal * C::EPSILON);
        createShadowPhotons(Ray(pos, pos + ray.direction), thread);
    }

    glm::dvec3 sampleRay(const Ray& ray, size_t ray_depth = 0)
    {
        Intersection intersect = scene->intersect(ray, true);

        if (!intersect) return scene->skyColor(ray);

        Ray new_ray(intersect.position);

        double n1 = ray.medium_ior;
        double n2 = abs(ray.medium_ior - scene->ior) < 1e-7 ? intersect.material->ior : scene->ior;

        glm::dvec3 emittance = (ray_depth == 0 || ray.specular) ? intersect.material->emittance : glm::dvec3(0.0);
        bool diffuse = ray_depth != 0 && !ray.specular;

        if (!diffuse && (intersect.material->perfect_mirror || Material::Fresnel(n1, n2, intersect.normal, -ray.direction) > Random::range(0, 1)))
        {
            /* SPECULAR REFLECTION */
            glm::dvec3 BRDF = intersect.material->SpecularBRDF();
            new_ray.reflectSpecular(ray.direction, intersect.normal, n1);

            glm::dvec3 indirect = sampleRay(new_ray, ray_depth + 1);
            return emittance + BRDF * indirect;
        }
        else
        {
            if (!diffuse && intersect.material->transparency > Random::range(0, 1))
            {
                /* SPECULAR REFRACTION */
                glm::dvec3 BRDF = intersect.material->SpecularBRDF();
                new_ray.refractSpecular(ray.direction, intersect.normal, n1, n2);

                glm::dvec3 indirect = sampleRay(new_ray, ray_depth + 1);
                return emittance + BRDF * indirect;
            }
            else
            {
                CoordinateSystem cs(intersect.normal);
                glm::dvec3 caustics = estimateCausticRadiance(intersect, -ray.direction, cs);
                if (ray_depth == 0 || ray.specular)
                {
                    double n1 = ray.medium_ior;
                    Ray new_ray(intersect.position);
                    new_ray.reflectDiffuse(cs, n1);
                    glm::dvec3 BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction)) * M_PI;

                    glm::dvec3 indirect = sampleRay(new_ray, ray_depth + 1);

                    glm::dvec3 direct = scene->sampleLights(intersect);

                    return emittance + caustics + BRDF * (direct + indirect);
                }
                else
                {
                    glm::dvec3 direct(0.0);
                    bool has_shadow_photon = !shadow_map.radiusSearch(intersect.position, photon_radius).empty();
                    auto direct_photons = direct_map.radiusSearch(intersect.position, photon_radius);

                    if (has_shadow_photon && direct_photons.empty())
                        direct = glm::dvec3(0.0);
                    else
                        direct = estimateRadiance(direct_photons, intersect, -ray.direction, cs);

                    glm::dvec3 indirect = estimateRadiance(indirect_map.radiusSearch(intersect.position, photon_radius), intersect, -ray.direction, cs);
                    return emittance + caustics + direct + indirect;
                }
            }
        }
    }

    glm::dvec3 estimateRadiance(
        const std::vector<Photon>& photons, 
        const Intersection& intersect, 
        const glm::dvec3& direction, 
        const CoordinateSystem& cs)
    {
        glm::dvec3 radiance(0.0);
        for (const auto& p : photons)
        {
            glm::dvec3 BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(p.direction), cs.globalToLocal(direction));
            radiance += p.flux * BRDF;

        }
        return radiance * (2.0 / pow2(photon_radius)); // 2pi / ( pi * r^2 )
    }

    glm::dvec3 estimateCausticRadiance(const Intersection& intersect, const glm::dvec3& direction, const CoordinateSystem& cs)
    {
        auto photons = caustic_map.radiusSearch(intersect.position, caustic_photon_radius);
        const double k = 1.0;
        glm::dvec3 ret(0.0);
        for (const auto& p : photons)
        {
            double wp = std::max(0.0, 1.0 - glm::distance(intersect.position, p.position) / (k * caustic_photon_radius));

            glm::dvec3 BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(p.direction), cs.globalToLocal(direction));
            ret += p.flux * BRDF * wp;
        }
        return ret * (2 / (pow2(caustic_photon_radius) * (1.0 - 2.0 / (3.0 * k)))); // 2pi / ( pi * r^2 * (1 - 2/3k) )
    }

    Octree<Photon> direct_map;
    Octree<Photon> indirect_map;
    Octree<Photon> caustic_map;
    Octree<ShadowPhoton> shadow_map;

    // Temporary photon maps which are filled by each thread in the first pass. The Octree can't handle
    // concurrent inserts, so this has to be done if multi-threading is to be used in the first pass.
    std::vector<std::vector<Photon>> direct_vecs;
    std::vector<std::vector<Photon>> indirect_vecs;
    std::vector<std::vector<Photon>> caustic_vecs;
    std::vector<std::vector<ShadowPhoton>> shadow_vecs;

    std::shared_ptr<Scene> scene;

    const double photon_radius = 0.1;
    const double caustic_photon_radius = 0.1;
    const double NON_CAUSTIC_REJECT = 0.01;

    const size_t max_ray_depth = 64; // Prevent call stack overflow, unlikely to ever happen.
};