#include "Camera.hpp"
#include "Random.hpp"

glm::dvec3 Camera::sampleRay(Ray ray, size_t ray_depth)
{
    if (ray_depth == max_ray_depth)
    {
        Log("Max ray depth reached.");
        return glm::dvec3(0.0);
    }

    Intersection intersect = scene->intersect(ray, true);

    if (!intersect)
    {
        return scene->skyColor(ray);
    }

    double terminate = 0.0;
    if (ray_depth > min_ray_depth)
    {
        terminate = 1.0 - intersect.material->reflect_probability;
        if (terminate > Random::range(0, 1))
        {
            return glm::dvec3(0.0);
        }
    }

    Ray new_ray(intersect.position);

    glm::dvec3 emittance = (ray_depth == 0 || ray.specular || naive) ? intersect.material->emittance : glm::dvec3(0);
    glm::dvec3 direct(0), indirect(0);
    glm::dvec3 BRDF;

    double n1 = ray.medium_ior;
    double n2 = abs(ray.medium_ior - scene->ior) < 1e-7 ? intersect.material->ior : scene->ior;

    if (intersect.material->perfect_mirror || Material::Fresnel(n1, n2, intersect.normal, -ray.direction) > Random::range(0,1))
    {
        /* SPECULAR REFLECTION */
        BRDF = intersect.material->SpecularBRDF();
        new_ray.reflectSpecular(ray.direction, intersect.normal, n1);
    }
    else
    {
        if (intersect.material->transparency > Random::range(0,1))
        {
            /* SPECULAR REFRACTION */
            BRDF = intersect.material->SpecularBRDF();
            new_ray.refractSpecular(ray.direction, intersect.normal, n1, n2);
        }
        else
        {
            /* DIFFUSE REFLECTION */
            CoordinateSystem cs(intersect.normal);

            new_ray.reflectDiffuse(cs, n1);
            BRDF = intersect.material->DiffuseBRDF(cs.globalToLocal(new_ray.direction), cs.globalToLocal(-ray.direction)) * M_PI;

            if (!naive) direct = scene->sampleLights(intersect);
        }
    }

    indirect = sampleRay(new_ray, ray_depth + 1);

    return (emittance + BRDF * (direct + indirect)) / (1.0 - terminate);
}

void Camera::samplePixel(size_t x, size_t y)
{
    double pixel_size = sensor_width / image.width;
    double sub_step = 1.0 / scene->sqrtspp;

    for (int s_x = 0; s_x < scene->sqrtspp; s_x++)
    {
        for (int s_y = 0; s_y < scene->sqrtspp; s_y++)
        {
            glm::dvec2 pixel_space_pos(x + s_x * sub_step + Random::range(0.0, sub_step), y + s_y * sub_step + Random::range(0.0, sub_step));
            glm::dvec2 center_offset = pixel_size * (glm::dvec2(image.width, image.height) / 2.0 - pixel_space_pos);

            glm::dvec3 sensor_pos = eye + forward * focal_length + left * center_offset.x + up * center_offset.y;

            image(x, y) += sampleRay(Ray(eye, sensor_pos, scene->ior));
        }
    }
    image(x, y) /= pow2(static_cast<double>(scene->sqrtspp));
}

void Camera::sampleImage(Scene& s)
{
    scene = std::make_shared<Scene>(s);

    std::function<void(Camera*, size_t, size_t)> f = &Camera::sampleImageThread;

    std::vector<std::unique_ptr<std::thread>> threads(std::thread::hardware_concurrency() - 2);
    for (size_t thread = 0; thread < threads.size(); thread++)
    {
        threads[thread] = std::make_unique<std::thread>(f, this, thread, threads.size());
    }

    std::function<void(Camera*)> p = &Camera::printInfoThread;
    std::thread print_thread(p, this);
    print_thread.detach();

    for (auto &thread : threads)
    {
        thread->join();
    }
}

void Camera::sampleImageThread(size_t thread, size_t num_threads)
{
    Random::seed(std::random_device{}()); // Each thread uses different seed.

    size_t step = (size_t)floor((double)image.width / num_threads);
    size_t start = thread * step;
    size_t end = thread != (num_threads - 1) ? start + step : image.width;
    
    for (size_t x = start; x < end; x++)
    {
        for (size_t y = 0; y < image.height; y++)
        {
            samplePixel(x, y);
            num_sampled_pixels++;
        }
    }
}

void Camera::printInfoThread()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        if (num_sampled_pixels == image.num_pixels)
            break;

        if (num_sampled_pixels != last_num_sampled_pixels)
        {
            size_t delta_pixels = num_sampled_pixels - last_num_sampled_pixels;
            size_t pixels_left = image.num_pixels - num_sampled_pixels;

            auto now = std::chrono::steady_clock::now();
            auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);

            times.push_back(static_cast<double>(delta_pixels) / delta_t.count());
            if (times.size() > num_times)
                times.pop_front();

            // moving average
            double pixels_per_msec = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

            double progress = 100.0 * static_cast<double>(num_sampled_pixels) / image.num_pixels;
            size_t msec_left = static_cast<size_t>(pixels_left / pixels_per_msec);
            size_t sps = static_cast<size_t>(pixels_per_msec * 1000.0 * pow2(static_cast<double>(scene->sqrtspp)));

            printProgressInfo(progress, msec_left, sps, std::cout);

            last_update = now;
            last_num_sampled_pixels = num_sampled_pixels;
        }
    }
}
