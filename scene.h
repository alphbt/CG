#pragma once
#ifndef __SCENE__
#define __SCENE__

#include "geometricObjects.h"
#include "light.h"
#include <vector>
#include <glm.hpp>

class Scene
{
public:
	std::vector<Sphere> spheres;
	std::vector<Light> lights;

	Scene() {}
	Scene(const Scene& s) : spheres(s.spheres), lights(s.lights) { }
	~Scene() { spheres.clear(); lights.clear(); }
};

#endif // !__SCENE__
