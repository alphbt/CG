#pragma once
#ifndef __MATERIAL__
#define __MATERIAL__

#include <glm.hpp>

class Material
{
public:

    Material(const glm::vec3& c, const float& spec,const glm::vec4& a,bool m = false) : color(c), specularExponent(spec), 
        albedo(a),isBump(m) {}
    Material() : color(), specularExponent(0), isBump(false),albedo(1,0,0,1) {}
    Material(const Material& material)
    {
        copy(material);
    }

    void copy(const Material& m)
    {
        color = m.color;
        specularExponent = m.specularExponent;
        isBump = m.isBump;
        albedo = m.albedo;
    }

    glm::vec4 albedo;
    glm::vec3 color;
    float specularExponent;
    bool isBump;

};

#endif // !__MATERIAL__