#include "surface.hpp"

#include <glm/gtc/type_ptr.hpp>

#include "../common/util.hpp"
#include "../common/constexpr-math.hpp"
#include "../common/constants.hpp"

Surface::Quadric::Quadric(const nlohmann::json &j, std::shared_ptr<Material> material)
    : Base(material)
{
    double XX = getOptional(j, "XX", 0.0);
    double XY = std::max(getOptional(j, "XY", 0.0), getOptional(j, "YX", 0.0)) / 2.0;
    double XZ = std::max(getOptional(j, "XZ", 0.0), getOptional(j, "ZX", 0.0)) / 2.0;
    double X  = getOptional(j, "X",  0.0) / 2.0;
    double YY = getOptional(j, "YY", 0.0);
    double YZ = std::max(getOptional(j, "YZ", 0.0), getOptional(j, "ZY", 0.0)) / 2.0;
    double Y  = getOptional(j, "Y",  0.0) / 2.0;
    double ZZ = getOptional(j, "ZZ", 0.0);
    double Z  = getOptional(j, "Z",  0.0) / 2.0;
    double R  = getOptional(j, "R",  0.0);

    double Qa[16] {
        XX, XY, XZ, X,
        XY, YY, YZ, Y,
        XZ, YZ, ZZ, Z,
         X,  Y,  Z, R
    };

    Q = glm::make_mat4(Qa);

    glm::dvec3 origin = getOptional(j, "origin", glm::dvec3(0.0));

    glm::dvec3 bound_dimensions = getOptional(j, "bound_dimensions", glm::dvec3(1.0));

    BB_ = BoundingBox(-bound_dimensions / 2.0, bound_dimensions / 2.0);

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

    double t_min, t_max;
    if (solveQuadratic(a, b, c, t_min, t_max) && t_max >= 0.0)
    {
        double t = t_bb + (t_min < 0.0 ? t_max : t_min);
        if (!BB_.contains(ray(t)))
        {
            return false;
        }
        intersection = Intersection(t);
        return true;
    }
    return false;
}

void Surface::Quadric::transform(const Transform &T)
{
    glm::dmat4 M_inv = glm::inverse(T.matrix);

    Q = glm::transpose(M_inv) * Q * M_inv;

    double Ga[12]{
        Q[0][0], Q[0][1], Q[0][2],
        Q[1][0], Q[1][1], Q[1][2],
        Q[2][0], Q[2][1], Q[2][2],
        Q[3][0], Q[3][1], Q[3][2]
    };

    G = 2.0 * glm::make_mat4x3(Ga);

    BB_.min += T.position;
    BB_.max += T.position;

    computeArea();
    computeBoundingBox();
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