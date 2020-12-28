#pragma once
#ifndef __LIGHT__
#define __LIGHT__

#include <glm.hpp>
#include <string>

class Light
{
public:
    Light(const glm::vec3& p, const float& i,const std::string t) : position(p), intensity(i), type(t) { }
    Light(const Light& l) : position(l.position), intensity(l.intensity), type(l.type) { }

    glm::vec3 position;
    float intensity;
    std::string type;
};

#endif // !__LIGHT__