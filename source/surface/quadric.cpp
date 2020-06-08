#include "surface.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>

#include "../common/util.hpp"
#include "../common/constants.hpp"

Surface::Quadric::Quadric(const nlohmann::json &j, std::shared_ptr<Material> material)
    : Base(material)
{
    std::vector<double> C = j.at("values");

    double Qa[16] {
        C[0], C[1], C[2], C[3],
        C[1], C[4], C[5], C[6],
        C[2], C[5], C[7], C[8],
        C[3], C[6], C[8], C[9]
    };

    Q = glm::make_mat4(Qa);

    glm::dvec3 origin = getOptional(j, "origin", glm::dvec3(0.0));
    double bound_width = getOptional(j, "bound_width", 6.0);

    BB = BoundingBox(origin - bound_width / 2.0, origin + bound_width / 2.0);

    glm::dmat4 M = glm::translate(glm::dmat4(1.0), origin);

    if (j.find("orientation") != j.end())
    {
        glm::dvec3 axis = glm::normalize(j.at("orientation").at("axis").get<glm::dvec3>());
        double angle = glm::radians(j.at("orientation").at("angle").get<double>());
        M = glm::rotate(M, angle, axis);
    }

    glm::dmat4 M_inv = glm::inverse(M);

    Q = glm::transpose(M_inv) * Q * M_inv;

    double Ga[12]{
        Q[0][0], Q[0][1], Q[0][2],
        Q[1][0], Q[1][1], Q[1][2],
        Q[2][0], Q[2][1], Q[2][2],
        Q[3][0], Q[3][1], Q[3][2]
    };

    G = 2.0 * glm::make_mat4x3(Ga);

    computeArea();
}

bool Surface::Quadric::intersect(const Ray& ray, Intersection& intersection) const
{
    double t_bb = 0.0;
    if (!BB.contains(ray.start) && !BB.intersect(ray, t_bb))
    {
        return false;
    }

    glm::dvec4 o(ray(t_bb), 1.0);
    glm::dvec4 d(ray.direction, 0.0);

    glm::dvec4 Qo = Q * o;
    double a = glm::dot(d, Q * d);
    double b = 2.0 * glm::dot(d, Qo);
    double c = glm::dot(o, Qo);

    double t;

    if (std::abs(a) < C::EPSILON)
    {
        t = -c / b;
    }
    else
    {
        double discriminant = pow2(b) - 4.0 * a*c;

        // Complex solution, no intersection
        if (discriminant <= C::EPSILON)
        {
            return false;
        }

        discriminant = std::sqrt(discriminant);

        t = (-b - discriminant) / (2.0 * a);
        if (t < C::EPSILON)
        {
            t = (-b + discriminant) / (2.0 * a);
        }
    }

    if (t < C::EPSILON)
    {
        return false;
    }

    t += t_bb;

    glm::dvec3 pos = ray(t);

    if (!BB.contains(pos))
    {
        return false;
    }

    intersection = Intersection(pos, normal(pos), t, material);
    
    return true;
}

glm::dvec3 Surface::Quadric::operator()(double u, double v) const
{
    return glm::dvec3();
}

glm::dvec3 Surface::Quadric::normal(const glm::dvec3& pos) const
{
    return glm::normalize(G * glm::dvec4(pos, 1.0));
}

BoundingBox Surface::Quadric::boundingBox() const
{
    return BB;
}

void Surface::Quadric::computeArea()
{
    area_ = 1.0;
}