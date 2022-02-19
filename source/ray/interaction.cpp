#include "interaction.hpp"

#include "../material/fresnel.hpp"
#include "../material/material.hpp"
#include "../sampling/sampling.hpp"
#include "../sampling/sampler.hpp"
#include "../common/constants.hpp"
#include "../common/coordinate-system.hpp"
#include "../surface/surface.hpp"
#include "../common/constexpr-math.hpp"

Interaction::Interaction(const Intersection &isect, const Ray &ray, double external_ior) :
    t(isect.t), ray(ray), out(-ray.direction), n1(ray.medium_ior),
    material(isect.surface->material), surface(isect.surface),
    position(ray(t)), normal(isect.surface->normal(position))
{
    double cos_theta = glm::dot(ray.direction, normal);

    inside = cos_theta > 0.0;
    n2 = (inside && !material->opaque) ? external_ior : material->ior;

    glm::dvec3 shading_normal = normal;
    if (isect.interpolate)
    {
        shading_normal = isect.surface->interpolatedNormal(isect.uv);
        if (cos_theta < 0.0 != glm::dot(ray.direction, shading_normal) < 0.0)
        {
            shading_normal = normal;
        }
    }

    if (cos_theta > 0.0)
    {
        normal = -normal;
        shading_normal = -shading_normal;
    }

    shading_cs = CoordinateSystem(shading_normal);

    // Specular reflect probability
    R = Fresnel::dielectric(n1, n2, glm::dot(shading_normal, out));

    // Transmission probability once ray has passed through specular layer
    T = material->transparency;

    if (material->rough_specular)
    {
        R = glm::clamp(R, 0.1, 0.9);
    }

    selectType();

    dirac_delta = type != DIFFUSE && !material->rough_specular;
}

bool Interaction::sampleBSDF(glm::dvec3& bsdf_absIdotN, double& pdf, Ray& new_ray, bool flux) const
{
    new_ray = Ray(*this);

    glm::dvec3 wi = shading_cs.to(new_ray.direction);
    
    if ((new_ray.refraction && wi.z >= 0.0) || (!new_ray.refraction && wi.z <= 0.0))
    {
        return false;
    }

    glm::dvec3 wo = shading_cs.to(out);

    bsdf_absIdotN = BSDF(wo, wi, pdf, flux, new_ray.dirac_delta) * std::abs(wi.z);

    return pdf > 0.0;
}

bool Interaction::BSDF(glm::dvec3& bsdf_absIdotN, const glm::dvec3& world_wi, double& pdf) const
{
    glm::dvec3 wi = shading_cs.to(world_wi);
    glm::dvec3 wo = shading_cs.to(out);

    bsdf_absIdotN = BSDF(wo, wi, pdf, false, false) * std::abs(wi.z);

    return pdf > 0.0;
}

glm::dvec3 Interaction::BSDF(const glm::dvec3& wo, const glm::dvec3& wi, double &pdf, bool flux, bool wi_dirac_delta) const
{
    double cos_theta = wo.z;
    if(material->rough_specular)
    {
        if (wi.z > 0.0)
        {
            cos_theta = glm::dot(wo, glm::normalize(wo + wi));
        }
        else
        {
            glm::dvec3 m = glm::normalize(wo * n1 + wi * n2);
            cos_theta = glm::dot(wo, m);
            if (n1 < n2) cos_theta = -cos_theta;
        }
    }

    // Full specular reflect probability
    if (material->perfect_mirror || material->complex_ior)
    {
        glm::dvec3 brdf = material->specularReflection(wi, wo, pdf);
        if (material->complex_ior)
        {
            brdf *= Fresnel::conductor(n1, material->complex_ior.get(), cos_theta);
        }
        return brdf;
    }

    // Full diffuse reflect probability
    if (n2 < 1.0)
    {
        return material->diffuseReflection(wi, wo, pdf);
    }

    double F = Fresnel::dielectric(n1, n2, cos_theta);

    double pdf_s, pdf_d;
    glm::dvec3 brdf_s = material->specularReflection(wi, wo, pdf_s);
    glm::dvec3 brdf_d = material->diffuseReflection(wi, wo, pdf_d);

    double pdf_t = pdf_s;
    glm::dvec3 btdf = brdf_s;
    if (F < 1.0)
    {
        btdf = material->specularTransmission(wi, wo, n1, n2, pdf_t, inside, flux);
    }
    
    if (wi_dirac_delta)
    {
        // wi guaranteed to be direction of ray spawned by interaction
        if (type == REFLECT)
        {
            pdf = R;
            return brdf_s * F;
        }
        else
        {
            pdf = T * (1.0 - R);
            return btdf * T * (1.0 - F);
        }
    }
    else if (!material->rough_specular)
    {
        pdf = pdf_d * (1.0 - R) * (1.0 - T);
        return brdf_d * (1.0 - F) * (1.0 - T);
    }

    pdf = glm::mix(glm::mix(pdf_d, pdf_t, T), pdf_s, R);
    return glm::mix(glm::mix(brdf_d, btdf, T), brdf_s, F);
}

// Selects the interaction type of the next ray spawned by interaction.
void Interaction::selectType()
{
    if (material->perfect_mirror || material->complex_ior)
    {
        type = REFLECT;
    }
    else if (n2 < 1.0)
    {
        type = DIFFUSE;
    }
    else
    {
        double p = Sampler::get<Dim::INTERACTION, 1>()[0];

        if (R > p)
        {
            type = REFLECT;
        }
        else if (R + (1.0 - R) * T > p)
        {
            type = REFRACT;
        }
        else // R + (1 - R) * T + (1 - R) * (1 - T) = 1 > p
        {
            type = DIFFUSE;
        }
    }
}

glm::dvec3 Interaction::specularNormal() const
{
    if (material->rough_specular)
    {
        auto u = Sampler::get<Dim::BSDF, 2>();
        return shading_cs.from(material->visibleMicrofacet(u[0], u[1], shading_cs.to(out)));
    }
    return shading_cs.normal;
}