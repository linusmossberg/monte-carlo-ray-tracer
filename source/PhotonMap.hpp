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
    Photon(const glm::vec3& flux, const glm::vec3& position, const glm::vec3& direction)
        : flux(flux), position(position), direction(direction) { }

    virtual glm::vec3 pos() const
    {
        return position;
    }

    glm::vec3 flux, position, direction;
};

// Separate shadow photon type to reduce memory usage and improve shadow photon search performance.
struct ShadowPhoton : public OctreeData
{
public:
    ShadowPhoton(const glm::vec3& position)
        : position(position) { }

    virtual glm::vec3 pos() const
    {
        return position;
    }

    glm::vec3 position;
};

class PhotonMap
{
public:
    PhotonMap(Scene& s, size_t photon_emissions, uint16_t max_node_data)
        : direct_map(glm::vec3(5,0,0), glm::vec3(8, 5, 6) + glm::vec3(0.01f), max_node_data),
          indirect_map(glm::vec3(5, 0, 0), glm::vec3(8, 5, 6) + glm::vec3(0.01f), max_node_data),
          caustic_map(glm::vec3(5, 0, 0), glm::vec3(8, 5, 6) + glm::vec3(0.01f), max_node_data),
          shadow_map(glm::vec3(5, 0, 0), glm::vec3(8, 5, 6) + glm::vec3(0.01f), max_node_data)
          
    {
        scene = std::make_shared<Scene>(s);

        struct EmissionWork
        {
            EmissionWork() { }

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
                work_vec.push_back(EmissionWork(light, emissions, photon_flux));
                count += emissions;
            }
        }

        WorkQueue<EmissionWork> work_queue(work_vec);

        std::vector<std::unique_ptr<std::thread>> threads(std::thread::hardware_concurrency() - 0);

        direct_vecs.resize(threads.size());
        indirect_vecs.resize(threads.size());
        caustic_vecs.resize(threads.size());
        shadow_vecs.resize(threads.size());

        for (size_t thread = 0; thread < threads.size(); thread++)
        {
            threads[thread] = std::make_unique<std::thread>([&, thread]()
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
            });
        }

        std::thread print_thread([&work_queue]()
        {
            while (true)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                double progress = work_queue.progress();
                std::cout << std::string("\rPhoton map " + formatProgress(progress) + " constructed.");

                if (abs(progress - 100.0) < 1e-3)
                {
                    break;
                }
            }
        });
        print_thread.detach();

        for (auto& thread : threads)
        {
            thread->join();
        }

        for (size_t thread = 0; thread < threads.size(); thread++)
        {
            for (const auto& p : direct_vecs[thread]) direct_map.insert(p);
            direct_vecs[thread].clear();

            for (const auto& p : indirect_vecs[thread]) indirect_map.insert(p);
            indirect_vecs[thread].clear();

            for (const auto& p : caustic_vecs[thread]) caustic_map.insert(p);
            caustic_vecs[thread].clear();

            for (const auto& p : shadow_vecs[thread]) shadow_map.insert(p);
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

                // Create shadow photons if refraction results in reflection due to critical angle
                if (new_ray.refractSpecular(ray.direction, intersect.normal, n1, n2))
                {
                    if (ray_depth == 0 && Random::range(0, 1) < NON_CAUSTIC_REJECT)
                    {
                        createShadowPhotons(Ray(intersect.position - intersect.normal * C::EPSILON, intersect.position + ray.direction), thread);
                    }
                }
            }
            else
            {
                /* DIFFUSE REFLECTION */
                if (ray_depth == 0)
                {
                    if (Random::range(0, 1) < NON_CAUSTIC_REJECT)
                    {
                        direct_vecs[thread].push_back(Photon(flux / NON_CAUSTIC_REJECT, intersect.position, ray.direction));
                        num_direct_photons++;
                        createShadowPhotons(Ray(intersect.position - intersect.normal * C::EPSILON, intersect.position + ray.direction), thread);
                    }
                }
                else
                {
                    if (ray.specular)
                    {
                        caustic_vecs[thread].push_back(Photon(flux, intersect.position, ray.direction));
                        num_caustic_photons++;
                    }
                    else
                    {
                        if (Random::range(0, 1) < NON_CAUSTIC_REJECT)
                        {
                            indirect_vecs[thread].push_back(Photon(flux / NON_CAUSTIC_REJECT, intersect.position, ray.direction));
                            num_indirect_photons++;
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

        // TODO: Might need to evaluate fresnel and transparency to probabilistically determine if shadow photon should be created
        if ((abs(intersect.material->transparency - 1.0) > C::EPSILON) && !intersect.material->perfect_mirror)
        {
            shadow_vecs[thread].push_back(ShadowPhoton(intersect.position));
            num_shadow_photons++;
        }

        glm::dvec3 pos(intersect.position - intersect.normal * C::EPSILON);
        createShadowPhotons(Ray(pos, pos + ray.direction), thread);
    }

    glm::dvec3 sampleRay(const Ray& ray, size_t ray_depth = 0)
    {
        Intersection intersect = scene->intersect(ray, true);

        if (!intersect) return scene->skyColor(ray);

        CoordinateSystem cs(intersect.normal);

        auto caustic_photons = caustic_map.radiusSearch(intersect.position, caustic_photon_radius);
        glm::dvec3 caustics = estimateRadiance(caustic_photons, intersect, -ray.direction, cs, caustic_photon_radius);

        if (ray_depth == 0)
        {
            double n1 = ray.medium_ior;
            Ray new_ray(intersect.position);
            new_ray.reflectDiffuse(cs, n1);
            glm::dvec3 BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction)) * M_PI;

            glm::dvec3 indirect = sampleRay(new_ray, ray_depth + 1);
            
            glm::dvec3 direct = scene->sampleLights(intersect);

            return caustics + BRDF * (direct + indirect);
        }
        else
        {
            glm::dvec3 direct(0.0);
            bool has_shadow_photon = !shadow_map.radiusSearch(intersect.position, photon_radius).empty();
            auto direct_photons = direct_map.radiusSearch(intersect.position, photon_radius);

            // Using shadow rays for ray depth > 0 doesen't seem to make a big difference
            if (has_shadow_photon && direct_photons.empty())
            {
                // shadow
                direct = glm::dvec3(0.0); 
            }
            else //if (!(has_shadow_photon && direct_photons.empty()))
            {
                // directly illuminated
                direct = estimateRadiance(direct_photons, intersect, -ray.direction, cs, photon_radius);
            }
            //else
            //{
            //    // unsure, use shadow ray
            //    //glm::dvec3 BRDF = intersect.material->DiffuseBRDF(Random::CosWeightedHemiSample(), cs.globalToLocal(-ray.direction)) * M_PI;
            //    //direct = scene->sampleLights(intersect) * BRDF;
            //}

            glm::dvec3 indirect = estimateRadiance(indirect_map.radiusSearch(intersect.position, photon_radius), intersect, -ray.direction, cs, photon_radius);
            return caustics + direct + indirect;
        }
    }

    glm::dvec3 estimateRadiance(const std::vector<Photon>& photons, const Intersection& intersect, const glm::dvec3& direction, const CoordinateSystem& cs, double radius)
    {
        glm::dvec3 ret(0.0);
        for (const auto& p : photons)
        {
            //double d = glm::distance(static_cast<glm::dvec3>(p.position), intersect.position);
            //double wp = std::max(0.0, 1.0 - d / (k * radius));

            glm::dvec3 BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(p.direction), cs.globalToLocal(direction));
            ret += static_cast<glm::dvec3>(p.flux) * BRDF /** wp*/;
        }
        const double tsts = C::TWO_PI / (C::PI * pow2(radius) /** (1.0 - 2.0 / (3.0 * k)) */ );
        ret *= tsts;
        return ret;
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

    const float photon_radius = 0.1f;
    const float caustic_photon_radius = 0.05f;
    const double NON_CAUSTIC_REJECT = 0.01;
    const double k = 1.0;

    const size_t max_ray_depth = 64; // Prevent call stack overflow, unlikely to ever happen.

    std::atomic_size_t num_direct_photons = 0;
    std::atomic_size_t num_indirect_photons = 0;
    std::atomic_size_t num_caustic_photons = 0;
    std::atomic_size_t num_shadow_photons = 0;
};