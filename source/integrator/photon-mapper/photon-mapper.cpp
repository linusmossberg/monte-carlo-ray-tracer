#include "photon-mapper.hpp"

#include <iostream>
#include <iomanip>
#include <thread>

#include <glm/gtx/component_wise.hpp>

#include "../../random/random.hpp"
#include "../../common/util.hpp"
#include "../../common/work-queue.hpp"
#include "../../common/constants.hpp"
#include "../../common/format.hpp"
#include "../../material/material.hpp"
#include "../../surface/surface.hpp"
#include "../../ray/interaction.hpp"

#include "../../octree/octree.cpp"
#include "../../octree/linear-octree.cpp"

PhotonMapper::PhotonMapper(const nlohmann::json& j) : Integrator(j)
{
    constexpr bool print = true;

    const nlohmann::json& pm = j.at("photon_map");

    double caustic_factor = pm.at("caustic_factor");
    size_t photon_emissions = pm.at("emissions");

    k_nearest_photons = getOptional(pm, "k_nearest_photons", 50);
    non_caustic_reject = 1.0 / caustic_factor;
    max_node_data = getOptional(pm, "max_photons_per_octree_leaf", 200);
    direct_visualization = getOptional(pm, "direct_visualization", false);

    photon_emissions = static_cast<size_t>(photon_emissions * caustic_factor);

    // Emissions per work
    constexpr size_t EPW = 100000;

    double total_add_flux = 0.0;
    for (const auto& light : scene.emissives)
    {
        total_add_flux += glm::compAdd(light->material->emittance * light->area());
    }

    struct EmissionWork
    {
        EmissionWork() : light(), num_emissions(0), photon_flux(0.0) { }
        EmissionWork(std::shared_ptr<Surface::Base> light, size_t num_emissions, const glm::dvec3& photon_flux)
            : light(light), num_emissions(num_emissions), photon_flux(photon_flux) { }

        std::shared_ptr<Surface::Base> light;
        size_t num_emissions;
        glm::dvec3 photon_flux;
    };

    std::vector<EmissionWork> work_vec;

    for (const auto& light : scene.emissives)
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

    std::shuffle(work_vec.begin(), work_vec.end(), Random::engine);
    WorkQueue<EmissionWork> work_queue(work_vec);

    std::vector<std::unique_ptr<std::thread>> threads(Integrator::num_threads);

    caustic_vecs.resize(threads.size());
    global_vecs.resize(threads.size());

    for (size_t thread = 0; thread < threads.size(); thread++)
    {
        threads[thread] = std::make_unique<std::thread>
        (
            [this, &work_queue, thread]()
            {
                EmissionWork work;
                while (work_queue.getWork(work))
                {
                    for (size_t i = 0; i < work.num_emissions; i++)
                    {
                        glm::dvec3 pos = (*work.light)(Random::unit(), Random::unit());
                        glm::dvec3 normal = work.light->normal(pos);
                        glm::dvec3 dir = CoordinateSystem::from(Random::cosWeightedHemiSample(), normal);

                        pos += normal * C::EPSILON;

                        emitPhoton(Ray(pos, pos + dir, scene.ior), work.photon_flux, thread);
                    }
                }
            }
        );
    }

    auto begin = std::chrono::high_resolution_clock::now();
    std::unique_ptr<std::thread> print_thread;
    if constexpr(print)
    {
        std::cout << std::endl << std::string(28, '-') << "| PHOTON MAPPING PASS |" << std::string(28, '-') 
                  << std::endl << std::endl << "Total number of photon emissions from light sources: " 
                  << Format::largeNumber(photon_emissions) << std::endl << std::endl;

        print_thread = std::make_unique<std::thread>([&work_queue]()
        {
            while (!work_queue.empty())
            {
                double progress = work_queue.progress();
                std::cout << std::string("\rPhotons emitted: " + Format::progress(progress));
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
    std::string duration = Format::timeDuration(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
    if constexpr(print)
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

    size_t num_global_photons = 0;
    size_t num_caustic_photons = 0;

    auto insertAndPop = [](auto& pvec, auto& pmap)
    {
        size_t removed_bytes = 0;
        auto i = pvec.end();
        while (i > pvec.begin())
        {
            if (removed_bytes > size_t(1e7))
            {
                pvec.shrink_to_fit();
                i = pvec.end();
                removed_bytes = 0;
            }
            i--;
            pmap.insert(*i);
            i = pvec.erase(i);
            removed_bytes += sizeof(Photon);
        }
        pvec.clear();
        pvec.shrink_to_fit();
    };

    BoundingBox BB = scene.BB();

    // Intermediate octrees that are converted to linear octrees once constructed.
    Octree<Photon> caustic_map_t(BB, max_node_data);
    Octree<Photon> global_map_t(BB, max_node_data);

    for (size_t thread = 0; thread < threads.size(); thread++)
    {
        num_global_photons += global_vecs[thread].size();
        insertAndPop(global_vecs[thread], global_map_t);

        num_caustic_photons += caustic_vecs[thread].size();
        insertAndPop(caustic_vecs[thread], caustic_map_t);
    }

    // Convert octrees to linear array representation
    caustic_map = LinearOctree<Photon>(caustic_map_t);
    global_map  = LinearOctree<Photon>(global_map_t);

    done_constructing_octrees = true;

    if constexpr(print)
    {
        print_thread->join();
        end = std::chrono::high_resolution_clock::now();
        std::string duration2 = Format::timeDuration(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
        std::cout << "\rPhotons emitted in " + duration + ". Octrees constructed in " + duration2 + "." << std::endl << std::endl
                  << "Photon maps and numbers of stored photons: " << std::endl << std::endl;

        std::cout << std::right
                  << std::setw(19) << "Global photons: "  << Format::largeNumber(num_global_photons)  << std::endl
                  << std::setw(19) << "Caustic photons: " << Format::largeNumber(num_caustic_photons) << std::endl;
    }
}

void PhotonMapper::emitPhoton(Ray ray, glm::dvec3 flux, size_t thread)
{
    RefractionHistory refraction_history(ray);
    glm::dvec3 bsdf_absIdotN;
    double bsdf_pdf;

    while (true)
    {
        Intersection intersection = scene.intersect(ray);

        if (!intersection)
        {
            return;
        }

        Interaction interaction(intersection, ray, refraction_history.externalIOR(ray));

        // Only spawn photons at locations that can produce non-dirac delta interactions.
        if (!interaction.material->dirac_delta)
        {
            if (ray.dirac_delta)
            {
                caustic_vecs[thread].emplace_back(flux, interaction.position, -ray.direction);
            }
            else if(Random::trial(non_caustic_reject))
            {
                global_vecs[thread].emplace_back(flux / non_caustic_reject, interaction.position, -ray.direction);
            }
        }

        if (!interaction.sampleBSDF(bsdf_absIdotN, bsdf_pdf, ray, true))
        {
            return;
        }

        bsdf_absIdotN /= bsdf_pdf;

        // Based on slide 13 of:
        // https://cgg.mff.cuni.cz/~jaroslav/teaching/2015-npgr010/slides/11%20-%20npgr010-2015%20-%20PM.pdf
        // I.e. reduce survival probability rather than flux to keep flux of spawned photons roughly constant.
        double survive = std::min(glm::compMax(bsdf_absIdotN), 0.95);
        if (survive == 0.0 || !Random::trial(survive))
        {
            return;
        }

        flux *= bsdf_absIdotN / survive;

        refraction_history.update(ray);
    }
}

glm::dvec3 PhotonMapper::sampleRay(Ray ray)
{
    glm::dvec3 radiance(0.0), throughput(1.0);
    RefractionHistory refraction_history(ray);
    glm::dvec3 bsdf_absIdotN;
    LightSample ls;

    while (true)
    {
        Intersection intersection = scene.intersect(ray);

        if (!intersection)
        {
            return radiance;
        }

        Interaction interaction(intersection, ray, refraction_history.externalIOR(ray));

        radiance += Integrator::sampleEmissive(interaction, ls) * throughput;

        if (interaction.dirac_delta)
        {
            if (!ray.dirac_delta && ray.depth != 0)
            {
                return radiance;
            }
            if (!interaction.sampleBSDF(bsdf_absIdotN, ls.bsdf_pdf, ray))
            {
                return radiance;
            }
            throughput *= bsdf_absIdotN / ls.bsdf_pdf;
        }
        else
        {
            radiance += estimateCausticRadiance(interaction) * throughput;

            if (!direct_visualization && (ray.dirac_delta || ray.depth == 0))
            {
                // Delay global evaluation
                radiance += Integrator::sampleDirect(interaction, ls) * throughput;
                if (!interaction.sampleBSDF(bsdf_absIdotN, ls.bsdf_pdf, ray))
                {
                    return radiance;
                }
                throughput *= bsdf_absIdotN / ls.bsdf_pdf;
            }
            else
            {
                // Global evaluation
                return radiance + estimateGlobalRadiance(interaction) * throughput;
            }
        }

        if (absorb(ray, throughput))
        {
            return radiance;
        }

        refraction_history.update(ray);
    }
}

glm::dvec3 PhotonMapper::estimateGlobalRadiance(const Interaction& interaction)
{
    auto photons = global_map.knnSearch(interaction.position, k_nearest_photons, global_radius_est.get());
    if (photons.empty())
    {
        return glm::dvec3(0.0);
    }

    global_radius_est.update(std::sqrt(photons.back().distance2));
    
    double bsdf_pdf;
    glm::dvec3 bsdf_absIdotN;
    glm::dvec3 radiance(0.0);
    for (const auto& p : photons)
    {
        if (interaction.BSDF(bsdf_absIdotN, p.data.dir(), bsdf_pdf))
        {
            radiance += p.data.flux() * bsdf_absIdotN / bsdf_pdf;
        }
    }
    return radiance / (photons.back().distance2 * C::PI);
}

/********************************************************************
Cone filtering method used for sharper caustics. Simplified for k = 1
*********************************************************************/
glm::dvec3 PhotonMapper::estimateCausticRadiance(const Interaction& interaction)
{
    auto photons = caustic_map.knnSearch(interaction.position, k_nearest_photons, caustic_radius_est.get());
    if (photons.empty())
    {
        return glm::dvec3(0.0);
    }

    caustic_radius_est.update(std::sqrt(photons.back().distance2));
    double inv_max_squared_radius = 1.0 / photons.back().distance2;

    double bsdf_pdf;
    glm::dvec3 bsdf_absIdotN;
    glm::dvec3 radiance(0.0);
    for (const auto& p : photons)
    {
        if (interaction.BSDF(bsdf_absIdotN, p.data.dir(), bsdf_pdf))
        {
            double wp = std::max(0.0, 1.0 - std::sqrt(p.distance2 * inv_max_squared_radius));
            radiance += (p.data.flux() * bsdf_absIdotN * wp) / bsdf_pdf;
        }
    }
    return 3.0 * radiance * inv_max_squared_radius * C::INV_PI;
}
