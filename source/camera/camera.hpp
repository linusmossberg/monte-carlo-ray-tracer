#pragma once

#include <iostream>
#include <thread>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <deque>
#include <numeric>
#include <functional>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <nlohmann/json.hpp>

#include "image.hpp"

#include "../ray/ray.hpp"
#include "../scene/scene.hpp"
#include "../integrator/integrator.hpp"
#include "../integrator/path-tracer/path-tracer.hpp"
#include "../integrator/photon-mapper/photon-mapper.hpp"
#include "../surface/surface.hpp"
#include "../random/random.hpp"
#include "../common/work-queue.hpp"
#include "../common/util.hpp"
#include "../common/intersection.hpp"
#include "../common/coordinate-system.hpp"
#include "../common/option.hpp"

struct Bucket
{
    Bucket() : min(0), max(0) { }
    Bucket(const glm::ivec2& min, const glm::ivec2& max) : min(min), max(max) { }

    glm::ivec2 min;
    glm::ivec2 max;
};

class Camera
{
public:
    Camera(const nlohmann::json &j, const Option &option) 
    {
        if (option.photon_map)
        {
            integrator = std::make_unique<PhotonMapper>(j);
        }
        else
        {
            integrator = std::make_unique<PathTracer>(j);
        }

        const nlohmann::json &c = j.at("cameras").at(option.camera_idx);

        image = Image(c.at("image"));
        eye = c.at("eye");
        focal_length = c.at("focal_length").get<double>() / 1000.0;
        sensor_width = c.at("sensor_width").get<double>() / 1000.0;
        sqrtspp = c.at("sqrtspp");
        savename = c.at("savename");
        aperture_radius = (focal_length / getOptional(c, "f_stop", -1.0)) / 2.0;
        focus_distance = getOptional(c, "focus_distance", -1.0);

        if (c.find("look_at") != c.end())
        {
            glm::dvec3 look_at = c.at("look_at");
            lookAt(look_at);
            if (focus_distance < 0.0)
            {
                focus_distance = glm::distance(eye, look_at);
            }
        }
        else
        {
            forward = glm::normalize(c.at("forward").get<glm::dvec3>());
            up = glm::normalize(c.at("up").get<glm::dvec3>());
            left = glm::normalize(glm::cross(up, forward));
        }
    }

    void capture();

    void sampleImage();

    void saveImage() const
    {
        image.save(savename);
    }

    void setPosition(const glm::dvec3& p)
    {
        eye = p;
    }

    void lookAt(const glm::dvec3& p)
    {
        forward = glm::normalize(p - eye);
        left = glm::normalize(glm::cross(glm::dvec3(0.0, 1.0, 0.0), forward));
        up = glm::normalize(glm::cross(forward, left));
    }

    size_t sqrtspp;

    glm::dvec3 eye;
    glm::dvec3 forward, left, up;

    double focal_length, sensor_width, aperture_radius, focus_distance;
    Image image;

    std::string savename;

private:
    void samplePixel(size_t x, size_t y);
    void sampleImageThread(WorkQueue<Bucket>& buckets);

    void printInfoThread(WorkQueue<Bucket>& buckets);

    const size_t bucket_size = 32;

    std::unique_ptr<Integrator> integrator;

    std::atomic_size_t num_sampled_pixels = 0;
    size_t last_num_sampled_pixels = 0;
    std::chrono::time_point<std::chrono::steady_clock> last_update = std::chrono::steady_clock::now();
    const size_t num_times = 32;
    std::deque<double> times;
};
