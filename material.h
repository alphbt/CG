#pragma once
#ifndef __MATERIAL__
#define __MATERIAL__

#include <glm.hpp>

class Material
{
public:

    Material(const glm::vec3& c, const float& spec, const float& i,bool m = false, bool r = false) : color(c), specularExponent(spec), refractiveIndex(i), 
        isBump(m), isReflect(r) {}
    Material() : refractiveIndex(1), color(), specularExponent(0), isBump(false),isReflect(false) {}
    Material(const Material& material)
    {
        copy(material);
    }

    void copy(const Material& m)
    {
        color = m.color;
        refractiveIndex = m.refractiveIndex;
        specularExponent = m.specularExponent;
        isBump = m.isBump;
    }

    float refractiveIndex;
    glm::vec3 color;
    float specularExponent;
    bool isBump;
    bool isReflect;

};

#endif // !__MATERIAL__