#include "scene.hpp"

glm::dvec3 Scene::sampleRay(Ray ray, size_t ray_depth)
{
    if (ray_depth == max_ray_depth)
    {
        Log("Bias introduced: Max ray depth reached in Scene::sampleRay()");
        return glm::dvec3(0.0);
    }

    Intersection intersection = intersect(ray, true);

    if (!intersection) return skyColor(ray);

    double terminate = 0.0;
    if (ray_depth > min_ray_depth)
    {
        terminate = 1.0 - intersection.material->reflect_probability;
        if (terminate > Random::range(0, 1))
        {
            return glm::dvec3(0.0);
        }
    }

    Ray new_ray(intersection.position);

    glm::dvec3 emittance = (ray_depth == 0 || ray.specular || naive) ? intersection.material->emittance : glm::dvec3(0);
    glm::dvec3 direct(0), indirect(0);
    glm::dvec3 BRDF;

    double n1 = ray.medium_ior;
    double n2 = abs(ray.medium_ior - ior) < C::EPSILON ? intersection.material->ior : ior;

    if (intersection.material->perfect_mirror || Material::Fresnel(n1, n2, intersection.normal, -ray.direction) > Random::range(0, 1))
    {
        /* SPECULAR REFLECTION */
        BRDF = intersection.material->SpecularBRDF();
        new_ray.reflectSpecular(ray.direction, intersection.normal, n1);
    }
    else
    {
        if (intersection.material->transparency > Random::range(0, 1))
        {
            /* SPECULAR REFRACTION */
            BRDF = intersection.material->SpecularBRDF();
            new_ray.refractSpecular(ray.direction, intersection.normal, n1, n2);
        }
        else
        {
            /* DIFFUSE REFLECTION */
            CoordinateSystem cs(intersection.normal);

            new_ray.reflectDiffuse(cs, n1);
            BRDF = intersection.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction)) * C::PI;

            if (!naive) direct = sampleDirect(intersection);
        }
    }

    indirect = sampleRay(new_ray, ray_depth + 1);

    return (emittance + BRDF * (direct + indirect)) / (1.0 - terminate);
}

Intersection Scene::intersect(const Ray& ray, bool align_normal, double min_distance)
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

/*************************************************************************************
Only applies cos(theta) from the rendering equation to the sampled point. This way  
direct and indirect (which uses cosine weighted sampling with PDF = cos(theta) / PI) 
can be treated the same way later, i.e. (direct + indirect) * (BRDF*PI). We also need 
to divide by PI to make (BRDF*PI)  applicable to the direct contribution as well.
**************************************************************************************/
glm::dvec3 Scene::sampleDirect(const Intersection& intersection)
{
    // Pick one light source and scale with 1/probability of picking light source
    if (!emissives.empty())
    {
        const auto& light = emissives[Random::uirange(0, emissives.size() - 1)];

        glm::dvec3 light_pos = light->operator()(Random::range(0, 1), Random::range(0, 1));
        Ray shadow_ray(intersection.position + intersection.normal * C::EPSILON, light_pos);

        double cos_theta = glm::dot(shadow_ray.direction, intersection.normal);

        if (cos_theta <= 0)
            return glm::dvec3(0.0);

        double min_distance = glm::distance(light_pos, intersection.position) - C::EPSILON;

        Intersection shadow_intersection = intersect(shadow_ray, false, min_distance);

        if (shadow_intersection)
        {
            double cos_light_theta = glm::dot(shadow_intersection.normal, -shadow_ray.direction);

            if (cos_light_theta <= 0 || glm::distance(shadow_intersection.position, light_pos) > C::EPSILON)
                return glm::dvec3(0.0);

            double t = light->area() * cos_light_theta / pow2(shadow_intersection.t);
            return (light->material->emittance * t * cos_theta * static_cast<double>(emissives.size())) / C::PI;
        }
    }
    return glm::dvec3(0.0);
}

void Scene::generateEmissives()
{
    for (const auto& surface : surfaces)
    {
        if (glm::length(surface->material->emittance) >= C::EPSILON)
        {
            surface->material->emittance /= surface->area(); // flux to radiosity
            emissives.push_back(surface);
        }
    }
}

BoundingBox Scene::boundingBox(bool recompute)
{
    if (surfaces.empty()) return BoundingBox();

    if (recompute || !bounding_box)
    {
        BoundingBox bb = surfaces.front()->boundingBox();
        for (const auto& surface : surfaces)
        {
            auto s_bb = surface->boundingBox();
            for (uint8_t c = 0; c < 3; c++)
            {
                bb.min[c] = std::min(bb.min[c], s_bb.min[c]);
                bb.max[c] = std::max(bb.max[c], s_bb.max[c]);
            }
        }
        bounding_box = std::make_unique<BoundingBox>(bb);
    }

    return *bounding_box;
}

glm::dvec3 Scene::skyColor(const Ray& ray) const
{
    double f = (glm::dot(glm::dvec3(0.0, 1.0, 0.0), ray.direction) * 0.7 + 1.0) / 2.0;
    if (f < 0.5)
    {
        return glm::dvec3(0.5); // ground color
    }
    return glm::dvec3(1.0 - f, 0.5, f);
}