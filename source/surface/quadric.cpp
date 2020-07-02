#include "surface.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "../common/util.hpp"
#include "../common/constants.hpp"

Surface::Quadric::Quadric(const nlohmann::json &j, std::shared_ptr<Material> material)
    : Base(material)
{
    double XX = getOptional(j, "XX", 0.0);
    double XY = getOptional(j, "XY", 0.0) / 2.0;
    double XZ = getOptional(j, "XZ", 0.0) / 2.0;
    double X  = getOptional(j, "X",  0.0) / 2.0;
    double YY = getOptional(j, "YY", 0.0);
    double YZ = getOptional(j, "YZ", 0.0) / 2.0;
    double Y  = getOptional(j, "Y",  0.0) / 2.0;
    double ZZ = getOptional(j, "ZZ", 0.0);
    double Z  = getOptional(j, "Z",  0.0) / 2.0;
    double R  = getOptional(j, "R",  0.0);

    if (std::abs(XY) < C::EPSILON) XY = getOptional(j, "YX", 0.0) / 2.0;
    if (std::abs(XZ) < C::EPSILON) XZ = getOptional(j, "ZX", 0.0) / 2.0;
    if (std::abs(YZ) < C::EPSILON) YZ = getOptional(j, "ZY", 0.0) / 2.0;

    double Qa[16] {
        XX, XY, XZ, X,
        XY, YY, YZ, Y,
        XZ, YZ, ZZ, Z,
         X,  Y,  Z, R
    };

    Q = glm::make_mat4(Qa);

    glm::dvec3 origin = getOptional(j, "origin", glm::dvec3(0.0));

    glm::dvec3 bound_dimensions;
    if (j.find("bound_dimensions") != j.end())
    {
        bound_dimensions = j.at("bound_dimensions");
    }
    else
    {
        double bound_width = getOptional(j, "bound_size", 1.0);
        bound_dimensions = glm::dvec3(bound_width);
    }
    BB_ = BoundingBox(origin - bound_dimensions / 2.0, origin + bound_dimensions / 2.0);

    glm::dmat4 scale = glm::scale(glm::dmat4(1.0), glm::dvec3(getOptional(j, "scale", 1.0)));
    glm::dmat4 translate = glm::translate(glm::dmat4(1.0), origin);
    glm::dmat4 rotate(1.0);

    if (j.find("orientation") != j.end())
    {
        glm::dvec3 axis = glm::normalize(j.at("orientation").at("axis").get<glm::dvec3>());
        double angle = glm::radians(j.at("orientation").at("angle").get<double>());
        rotate = glm::rotate(rotate, angle, axis);
    }

    glm::dmat4 M = translate * rotate * scale;

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
    computeBoundingBox();
}

/**********************************************************************
 Ray equation: r = o + d*t
 Quadric equation: transpose(p)*Q*p = 0
 Ray quadric intersection: transpose(r)*Q*r = 0
 
   transpose(r)*Q*r = dot(r,Q*r) =
 = dot(o + d*t, Q * ((o + d*t))) = 
 = dot(d, Q*d) * t^2 + (dot(o, Q*d) + dot(d, Q*o)) * t + dot(o, Q*o) =
 = dot(d, Q*d) * t^2 + 2*dot(d, Q*o) * t + dot(o, Q*o) = 0
 
 i.e. if we set:
 a = dot(d, Q*d)
 b = dot(d, Q*o) * 2
 c = dot(o, Q*o)
 
 then we can find eventual ray intersections by solving the quadratic equation: 
 a*t^2 + b*t + c = 0
/**********************************************************************/
bool Surface::Quadric::intersect(const Ray& ray, Intersection& intersection) const
{
    // Intersect with bounding box and start at this 
    // intersection to render the sliced quadric correctly.
    double t_bb = 0.0;
    if (!BB_.intersect(ray, t_bb))
    {
        return false;
    }

    glm::dvec4 o(ray(t_bb), 1.0);
    glm::dvec4 d(ray.direction, 0.0);

    glm::dvec4 Qo = Q * o;

    double a = glm::dot(d, Q * d);
    double b = glm::dot(d, Qo) * 2.0;
    double c = glm::dot(o, Qo);

    double t;
    if (std::abs(a) < C::EPSILON)
    {
        t = -c / b;
    }
    else
    {
        double discriminant = pow2(b) - 4.0 * a*c;

        // Complex solutions, no intersection
        if (discriminant < 0.0)
        {
            return false;
        }

        double v = std::sqrt(discriminant);

        double t0 = (-b - v) / a;
        double t1 = (-b + v) / a;

        if (t0 > t1) std::swap(t0, t1);

        t = t0 < 0.0 ? t1 : t0;

        t /= 2.0;
    }

    if (t < 0.0) return false;

    t += t_bb;

    if (!BB_.contains(ray(t)))
    {
        return false;
    }

    intersection = Intersection(t);
    
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

void Surface::Quadric::computeArea()
{
    area_ = 1.0;
}