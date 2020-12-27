#pragma once
#ifndef __LIGHT__
#define __LIGHT__

#include <glm.hpp>

class Light
{
public:
    Light(const glm::vec3& p, const float& i) : position(p), intensity(i) {}

    glm::vec3 position;
    float intensity;
};

#endif // !__LIGHT__