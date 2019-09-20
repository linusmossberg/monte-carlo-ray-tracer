#pragma once

#include <vector>
#include <memory>

#include "Surface.h"
#include "Ray.h"

class Scene
{
public:
	Intersection intersect(const Ray& ray, Surface::Base* &ignore)
	{
		Intersection intersect;
		Surface::Base* next_ignore = nullptr;
		for (const auto& surface : surfaces)
		{
			if (surface.get() != ignore)
			{
				Intersection t_intersect;
				if (surface->intersect(ray, t_intersect))
				{
					if (t_intersect.t < intersect.t)
					{
						intersect = t_intersect;
						next_ignore = surface.get();
					}
				}
			}
		}
		ignore = next_ignore;
		return intersect;
	}

	std::vector<std::shared_ptr<Surface::Base>> surfaces;
};