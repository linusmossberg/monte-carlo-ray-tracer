#pragma once

#include <vector>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vector_relational.hpp>

#include "Util.hpp"
#include "Scene.hpp"
#include "Octree.hpp"
#include "WorkQueue.hpp"
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

// Separate shadow photon type to reduce memory usage
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
    PhotonMap(std::shared_ptr<Scene> s, size_t photon_emissions, uint16_t max_node_data, double caustic_factor, double radius, double caustic_radius, bool print = true)
        : scene(s), non_caustic_reject(1.0 / caustic_factor), radius(radius), caustic_radius(caustic_radius), max_node_data(max_node_data),
          indirect_map(s->boundingBox(1), max_node_data),
            direct_map(s->boundingBox(0), max_node_data),
           caustic_map(s->boundingBox(0), max_node_data),
            shadow_map(s->boundingBox(0), max_node_data)
          
    {
        struct EmissionWork
        {
            EmissionWork() : light(), num_emissions(0), photon_flux(0.0) { }

            EmissionWork(std::shared_ptr<Surface::Base> light, size_t num_emissions, const glm::dvec3& photon_flux)
                : light(light), num_emissions(num_emissions), photon_flux(photon_flux) { }

            std::shared_ptr<Surface::Base> light;
            size_t num_emissions;
            glm::dvec3 photon_flux;
        };

        photon_emissions = static_cast<size_t>(photon_emissions * caustic_factor);

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

        std::vector<std::unique_ptr<std::thread>> threads(scene->num_threads);

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
                            glm::dvec3 dir = CoordinateSystem::localToGlobal(Random::CosWeightedHemiSample(), normal);

                            pos += normal * C::EPSILON;

                            emitPhoton(Ray(pos, pos + dir, scene->ior), work.photon_flux, thread);
                        }
                    }
                }
            );
        }

        auto begin = std::chrono::high_resolution_clock::now();
        std::unique_ptr<std::thread> print_thread;
        if (print)
        {
            std::cout << std::string(28, '-') << "| PHOTON MAPPING PASS |" << std::string(28, '-') << std::endl << std::endl;
            std::cout << "Total number of photon emissions from light sources: " << formatLargeNumber(photon_emissions) << std::endl << std::endl;
            print_thread = std::make_unique<std::thread>([&work_queue]()
            {
                while (!work_queue.empty())
                {
                    double progress = work_queue.progress();
                    std::cout << std::string("\rPhotons emitted: " + formatProgress(progress));
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            });
            print_thread->join();
        }

        for (auto& thread : threads)
        {
            thread->join();
        }

        std::atomic<bool> done_constructing_octrees = false;
        auto end = std::chrono::high_resolution_clock::now();
        std::string duration = formatTimeDuration(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
        if (print)
        {
            std::string info = "\rPhotons emitted in " + duration + ". Constructing octrees";
            std::cout << info;
            begin = std::chrono::high_resolution_clock::now();
            
            print_thread = std::make_unique<std::thread>([&done_constructing_octrees, info]()
            {
                std::string dots("");
                int i = 0;
                while (!done_constructing_octrees)
                {
                    std::cout << "\r" + std::string(60, ' ') + info + dots;
                    dots += ".";
                    if (i != 0 && i % 3 == 0) dots = ".";
                    i++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(800));
                }
            });
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

        done_constructing_octrees = true;
        if(print) print_thread->join();

        if (print)
        {
            end = std::chrono::high_resolution_clock::now();
            std::string duration2 = formatTimeDuration(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
            std::cout << "\rPhotons emitted in " + duration + ". Octrees constructed in " + duration2 + "." << std::endl << std::endl
                      << "Photon maps and numbers of stored photons: " << std::endl << std::endl;

            std::cout << std::right
                      << std::setw(19) << "Direct photons: "   << formatLargeNumber(num_direct_photons)   << std::endl
                      << std::setw(19) << "Indirect photons: " << formatLargeNumber(num_indirect_photons) << std::endl
                      << std::setw(19) << "Caustic photons: "  << formatLargeNumber(num_caustic_photons)  << std::endl
                      << std::setw(19) << "Shadow photons: "   << formatLargeNumber(num_shadow_photons)   << std::endl << std::endl;
        }
    }

    void emitPhoton(const Ray& ray, const glm::dvec3& flux, size_t thread, size_t ray_depth = 0)
    {
        if (ray_depth == max_ray_depth)
        {
            Log("Bias introduced: Max ray depth reached in PhotonMap::emitPhoton()");
            return;
        }

        Intersection intersect = scene->intersect(ray, true);

        if (!intersect) return;

        // Delay path termination until any new photons have been stored.
        double terminate = ray_depth > 0 ? 1.0 - intersect.material->reflect_probability : 0.0;
        bool should_terminate = terminate > Random::range(0, 1);

        Ray new_ray(intersect.position);

        glm::dvec3 BRDF;

        double n1 = ray.medium_ior;
        double n2 = abs(ray.medium_ior - scene->ior) < C::EPSILON ? intersect.material->ior : scene->ior;

        if (intersect.material->perfect_mirror || Material::Fresnel(n1, n2, intersect.normal, -ray.direction) > Random::range(0, 1))
        {
            /* SPECULAR REFLECTION */
            if (ray_depth == 0 && Random::range(0, 1) < non_caustic_reject)
            {
                createShadowPhotons(Ray(intersect.position - intersect.normal * C::EPSILON, intersect.position + ray.direction), thread);
            }

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
            }
            else
            {
                /* DIFFUSE REFLECTION */
                if (ray_depth == 0)
                { 
                    if (Random::range(0, 1) < non_caustic_reject)
                    {
                        direct_vecs[thread].emplace_back(flux / non_caustic_reject, intersect.position, ray.direction);
                        createShadowPhotons(Ray(intersect.position - intersect.normal * C::EPSILON, intersect.position + ray.direction), thread);
                    }
                }
                else
                {
                    if (ray.specular)
                    {
                        caustic_vecs[thread].emplace_back(flux, intersect.position, ray.direction);
                    }
                    else if(Random::range(0, 1) < non_caustic_reject)
                    {
                        indirect_vecs[thread].emplace_back(flux / non_caustic_reject, intersect.position, ray.direction);
                    }
                }

                if (should_terminate) return;

                CoordinateSystem cs(intersect.normal);
                new_ray.reflectDiffuse(cs, n1);
                BRDF = C::PI * intersect.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction));
            }
        }
        emitPhoton(new_ray, flux * BRDF / (1.0 - terminate), thread, ray_depth + 1);
    }

    void createShadowPhotons(const Ray& ray, size_t thread)
    {
        Intersection intersect = scene->intersect(ray, true);

        if (!intersect) return;

        if (intersect.material->can_diffusely_reflect)
        {
            shadow_vecs[thread].emplace_back(intersect.position);
        }

        glm::dvec3 pos(intersect.position - intersect.normal * C::EPSILON);
        createShadowPhotons(Ray(pos, pos + ray.direction), thread);
    }

    glm::dvec3 sampleRay(const Ray& ray, size_t ray_depth = 0)
    {
        if (ray_depth == max_ray_depth)
        {
            Log("Bias introduced: Max ray depth reached in PhotonMap::sampleRay()");
            return glm::dvec3(0.0);
        }

        Intersection intersect = scene->intersect(ray, true);

        if (!intersect) return scene->skyColor(ray);

        double terminate = 0.0;
        if (ray_depth > 3)
        {
            terminate = 1.0 - intersect.material->reflect_probability;
            if (terminate > Random::range(0, 1))
            {
                return glm::dvec3(0.0);
            }
        }

        Ray new_ray(intersect.position);

        double n1 = ray.medium_ior;
        double n2 = abs(ray.medium_ior - scene->ior) < C::EPSILON ? intersect.material->ior : scene->ior;

        bool diffuse = ray_depth != 0 && !ray.specular;
        bool global_contribution_evaluated = false;

        glm::dvec3 emittance = (ray_depth == 0 || ray.specular) ? intersect.material->emittance : glm::dvec3(0.0);
        glm::dvec3 photon_map_contrib(0.0), direct(0.0), indirect(0.0), BRDF(0.0);

        if (!diffuse && (intersect.material->perfect_mirror || Material::Fresnel(n1, n2, intersect.normal, -ray.direction) > Random::range(0, 1)))
        {
            /* SPECULAR REFLECTION */
            BRDF = intersect.material->SpecularBRDF();
            new_ray.reflectSpecular(ray.direction, intersect.normal, n1);
        }
        else
        {
            if (!diffuse && intersect.material->transparency > Random::range(0, 1))
            {
                /* SPECULAR REFRACTION */
                BRDF = intersect.material->SpecularBRDF();
                new_ray.refractSpecular(ray.direction, intersect.normal, n1, n2);
            }
            else
            {
                /* DIFFUSE REFLECTION */
                CoordinateSystem cs(intersect.normal);

                //photon_map_contrib = estimateCausticRadiance(intersect, -ray.direction, cs);
                photon_map_contrib += estimateRadiance(caustic_map.radiusSearch(intersect.position, caustic_radius), intersect, -ray.direction, cs, caustic_radius);

                bool has_shadow_photons = hasShadowPhoton(intersect);
                if (ray_depth == 0 || ray.specular || has_shadow_photons)
                {
                    double n1 = ray.medium_ior;
                    new_ray.reflectDiffuse(cs, n1);
                    BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction)) * C::PI;
                    direct = sampleDirect(intersect, has_shadow_photons, false);
                }
                else
                {
                    photon_map_contrib += estimateRadiance(direct_map.radiusSearch(intersect.position, radius), intersect, -ray.direction, cs, radius);
                    photon_map_contrib += estimateRadiance(indirect_map.radiusSearch(intersect.position, radius), intersect, -ray.direction, cs, radius);
                    global_contribution_evaluated = true;
                }
            }
        }

        // Terminate the path once the global contribution has been evaluated to estimate all incoming radiance at this point. This happens
        // once a diffusely reflected ray hits a point that evaluates to diffuse, provided that there are no shadow photons at this point.
        if (!global_contribution_evaluated)
        {
            indirect = sampleRay(new_ray, ray_depth + 1);
        }
        return (emittance + photon_map_contrib + (indirect + direct) * BRDF) / (1.0 - terminate);
    }

    glm::dvec3 estimateRadiance(
        const std::vector<Photon>& photons, 
        const Intersection& intersect, 
        const glm::dvec3& direction, 
        const CoordinateSystem& cs,
        double r) const
    {
        glm::dvec3 radiance(0.0);
        for (const auto& p : photons)
        {
            if (-glm::dot(p.direction, cs.normal) <= 0.0) continue;
            glm::dvec3 BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(p.direction), cs.globalToLocal(direction));
            radiance += p.flux * BRDF;
        }
        return radiance / pow2(r);
    }

    /*******************************************************************************
    Currently unused cone filtering method that can be used for sharper caustics. 
    This seems to produce hot spots in places with sparse caustic photons however, 
    which appears as bright blobs the size of the caustic_radius in places where 
    you wouldn't expect to see high intensity caustics.
    *******************************************************************************/
    glm::dvec3 estimateCausticRadiance(const Intersection& intersect, const glm::dvec3& direction, const CoordinateSystem& cs) const
    {
        auto photons = caustic_map.radiusSearch(intersect.position, caustic_radius);
        const double k = 1.0;
        glm::dvec3 radiance(0.0);
        for (const auto& p : photons)
        {
            if (-glm::dot(p.direction, cs.normal) <= 0.0) continue;
            double wp = std::max(0.0, 1.0 - glm::distance(intersect.position, p.position) / (k * caustic_radius));
            glm::dvec3 BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(p.direction), cs.globalToLocal(direction));
            radiance += p.flux * BRDF * wp;
        }
        return radiance / (pow2(caustic_radius) * (1.0 - 2.0 / (3.0 * k)));
    }

    bool hasShadowPhoton(const Intersection& intersect) const
    {
        return !shadow_map.radiusSearch(intersect.position, radius).empty();
    }

    glm::dvec3 sampleDirect(const Intersection& intersect, bool has_shadow_photons, bool use_direct_map) const
    {
        /**************************************************************************************************************************
        If enabled, this reduces the number of shadow rays used at points that most likely aren't under direct illumination without
        introducing much artifacts. Doing a radius search in the octree is about as fast as sampling a shadow ray in my testing 
        however, but this might not be the case when using several light sources or with a faster radius empty query method.
        **************************************************************************************************************************/
        if(use_direct_map && has_shadow_photons && direct_map.radiusSearch(intersect.position, radius).empty())
        {
            return glm::dvec3(0.0);
        }
        return scene->sampleDirect(intersect);
    }

    // Implemented in Tests.cpp
    void test(std::ostream& log, size_t num_iterations) const;

private:

    Octree<Photon> caustic_map;
    Octree<Photon> direct_map;       // <-
    Octree<Photon> indirect_map;     // <- These are commonly combined into a global photon map
    Octree<ShadowPhoton> shadow_map; // <-

    // Temporary photon maps which are filled by each thread in the first pass. The Octree can't handle
    // concurrent inserts, so this has to be done if multi-threading is to be used in the first pass.
    std::vector<std::vector<Photon>> caustic_vecs;
    std::vector<std::vector<Photon>> direct_vecs;
    std::vector<std::vector<Photon>> indirect_vecs;
    std::vector<std::vector<ShadowPhoton>> shadow_vecs;

    std::shared_ptr<Scene> scene;

    double radius;
    double caustic_radius;
    double non_caustic_reject;

    uint16_t max_node_data;

    const size_t max_ray_depth = 64; // Prevent call stack overflow, unlikely to ever happen.
};