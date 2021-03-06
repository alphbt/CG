#pragma once
#ifndef __GEOMOBJ__
#define __GOEMOBJ__

#include "Ray.h"
#include "Material.h"
#include <vector>
#include <string>

class Sphere
{
public:
	Sphere() {}
	Sphere(const glm::vec3& c, const float& r, const Material& m,const std::string t = "none") : center(c), radius(r), material(m),type(t) {}
	Sphere(const Sphere& sphere) : center(sphere.center), radius(sphere.radius), material(sphere.material), type(sphere.type) {}
	bool hit(const Ray& ray, float& tMin) const;

	glm::vec3 center;
	float radius;
	Material material;
	std::string type;
	static const float eps;
};

const float Sphere::eps = 0.001;

bool Sphere::hit(const Ray& ray, float& tMin) const
{
	glm::vec3 l = center - ray.origin;
	float l2 = glm::dot(l, l); //������� l
	float tca = glm::dot(l, ray.direction); //���������� �� ���� �� ������
	float d2 = l2 - tca * tca;
	if (d2 > radius * radius)
		return false;
	else
	{
		float thc = sqrtf(radius * radius - d2);
		float t2;
		if (tca < thc)
		{
			tMin = tca + thc;
			t2 = tca - thc;
		}
		else
		{
			tMin = tca - thc;
			t2 = tca + thc;
		}

		if (std::fabs(tMin) < eps) tMin = t2;
		
		return tMin > eps;
	}
}
#endif // !__GEOMOBJ__