#ifndef __RAY__
#define __RAY__

#include <glm.hpp>

class Ray
{
public:
	glm::vec3 origin;
	glm::vec3 direction;

	Ray() : origin(glm::vec3(0.0f, 0.0f, 0.0f)), direction(glm::vec3(0.0f, 0.1f, 0.0f)) {}
	Ray(const glm::vec3& o, const glm::vec3& dir) : origin(o), direction(dir) {}
	Ray(const Ray& ray) : origin(ray.origin), direction(ray.direction) {}

	Ray& operator=(const Ray& rhs);
};

Ray& Ray::operator= (const Ray& rhs)
{
	if (this == &rhs)
		return (*this);

	origin = rhs.origin;
	direction = rhs.direction;

	return (*this);
}

#endif // !__RAY__
