#pragma once
#ifndef __MATERIAL__
#define __MATERIAL__

#include <glm.hpp>

class Material
{
public:

    Material(const glm::vec3& c, const float& spec, const float& i) : color(c), specularExponent(spec), refractiveIndex(i) {}
    Material() : refractiveIndex(1), color(), specularExponent() {}
    Material(const Material& material)
    {
        copy(material);
    }

    void copy(const Material& m)
    {
        this->color = m.color;
        this->refractiveIndex = m.refractiveIndex;
        this->specularExponent = m.specularExponent;
    }

    float refractiveIndex;
    glm::vec3 color;
    float specularExponent;
};

#endif // !__MATERIAL__