#include "path-tracer.hpp"

#include "../../common/util.hpp"
#include "../../random/random.hpp"
#include "../../common/constants.hpp"

/*************************************************************************************************************
A material can be any combination of reflective, transparent and diffuse, but instead of branching into several
paths only one is selected probabilistically based on the fresnel and transparency at the intersection point.
Energy is conserved 'automatically' because the path probability is set to be the same as the path weight.
*************************************************************************************************************/
glm::dvec3 PathTracer::sampleRay(Ray ray, size_t ray_depth)
{
    if (ray_depth == Integrator::max_ray_depth)
    {
        Log("Bias introduced: Max ray depth reached in Scene::sampleRay()");
        return glm::dvec3(0.0);
    }

    Intersection intersect = scene.intersect(ray);

    if (!intersect)
    {
        return scene.skyColor(ray);
    }

    double absorb = ray_depth > Integrator::min_ray_depth ? 1.0 - intersect.material->reflect_probability : 0.0;

    if (Random::trial(absorb))
    {
        return glm::dvec3(0.0);
    }

    Ray new_ray(intersect.position);

    glm::dvec3 BRDF;
    glm::dvec3 direct(0);
    glm::dvec3 emittance = (ray_depth == 0 || ray.specular || naive) ? intersect.material->emittance : glm::dvec3(0);

    double n1 = ray.medium_ior;
    double n2 = std::abs(ray.medium_ior - scene.ior) < C::EPSILON ? intersect.material->ior : scene.ior;

    switch (intersect.selectNewPath(n1, n2, -ray.direction))
    {
        case Path::REFLECT:
        {
            BRDF = intersect.material->SpecularBRDF();
            new_ray.reflectSpecular(ray.direction, intersect, n1);
            break;
        }
        case Path::REFRACT:
        {
            BRDF = intersect.material->SpecularBRDF();
            new_ray.refractSpecular(ray.direction, intersect, n1, n2);
            break;
        }
        case Path::DIFFUSE:
        {
            CoordinateSystem cs(intersect.shadingNormal());
            new_ray.reflectDiffuse(cs, intersect, n1);
            BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction)) * C::PI;
            if (!naive) direct = Integrator::sampleDirect(intersect);
            break;
        }
    }
    glm::dvec3 indirect = sampleRay(new_ray, ray_depth + 1);

    return (emittance + BRDF * (direct + indirect)) / (1.0 - absorb);
}