#include <glm.hpp>
#include <gtc/constants.hpp>
#include "scene.h"
#include "ray.h"
#include "material.h"
#include <algorithm>
#include <iostream>
#include "image.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stbi_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define _CRT_SECURE_NO_WARNINGS

static const float kInfinity = std::numeric_limits<float>::max();
static const glm::vec3 kDefaultBackgroundColor = glm::vec3(0.235294, 0.67451, 0.843137);
int x = 1270;
int y = 770;
int n = 3;
unsigned char* barknmpImage = stbi_load("Bark_NRM.jpg", &x, &y, &n, 0);
Image barkNMP(barknmpImage, x, y);
unsigned char* barkImage = stbi_load("Bark.jpg", &x, &y, &n, 0);
Image bark(barkImage, x, y);

glm::vec3 reflect(const glm::vec3& I, const glm::vec3& N)
{
    return N * glm::dot(N, I) * 2.0f - I;
}

glm::vec3 refract(glm::vec3& I, glm::vec3& N, float& refractiveIndex)
{
    float cosi = glm::clamp(-1.0f, 1.0f, glm::dot(I, N));
    float etai = 1, etat = refractiveIndex;
    glm::vec3 n = N;
    if (cosi < 0)
        cosi = -cosi;
    else
    {
        std::swap(etai, etat);
        n = -N;
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? glm::vec3(0.0f, 0.0f, 0.0f) : eta * I + (eta * cosi - sqrtf(k)) * n;
}

void fresnel(const glm::vec3& I, const glm::vec3& N, const float& ior, float& kr)
{
    float cosi = glm::clamp(-1.0f, 1.0f, glm::dot(I, N));
    float etai = 1, etat = ior;

    if (cosi > 0)
    {
        std::swap(etai, etat);
    }

    float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));

    if (sint >= 1) 
    {
        kr = 1;
    }
    else
    {
        float cost = sqrtf(std::max(0.f, 1 - sint * sint));
        cosi = fabsf(cosi);
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }
}

void getSphereTextureCoordinats(const glm::vec3& p, float& u, float& v) {
    float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    float phi = std::atan2(-p.z,p.x);
    u = (phi + glm::pi<float>()) / (2 * glm::pi<float>());
    float theta = glm::acos(-p.y / r);
    v = theta / glm::pi<float>();
}

bool SceneIntersect(const Ray& ray, const std::vector<Sphere>& spheres, glm::vec3& hit,
    glm::vec3& normal, Material& material,Sphere& sphere)
{
    float spheres_dist = kInfinity;
    for (int i = 0; i < spheres.size(); i++)
    {
        float dist;
        if (spheres[i].hit(ray, dist) && dist < spheres_dist)
        {
            spheres_dist = dist;
            hit = ray.origin + ray.direction * dist;
            material.copy(spheres[i].material);
            normal = hit - spheres[i].center;
            if (spheres[i].material.isBump)
            {
                float u, v;
                getSphereTextureCoordinats(normal, u, v);
                normal = barkNMP.value(u, v);
                material.color = bark.value(u,v) ;
            }  
            normal = glm::normalize(normal);
            sphere = spheres[i];
        }
    }

    float checkerboard_dist = kInfinity;
    if (fabs(ray.direction.y) > 1e-3) 
    {
        float d = -(ray.origin.y + 4) / ray.direction.y; // the checkerboard plane has equation y = -4
        glm::vec3 pt = ray.origin + ray.direction * d;
        if (d > 0 && fabs(pt.x) < 10 && pt.z<-10 && pt.z>-30 && d < spheres_dist) {
            checkerboard_dist = d;
            hit = pt;
            normal = glm::vec3(0, 1, 0);
            material.color = (int(.5 * hit.x + 1000) + int(.5 * hit.z)) & 1 ? glm::vec3(1, 1, 1) : glm::vec3(1, .7, .3);
            material.color = material.color * 0.3f;
        }
    }

    return std::min(spheres_dist, checkerboard_dist) < 1000;
}

void Lighting(const Scene& scene,glm::vec3& normal, glm::vec3& hitPoint,
    const glm::vec3& v,float& specularExp,float& diffuse, float& specular, float& back)
{
    std::for_each(scene.lights.begin(),scene.lights.end(), [&](auto& light)
        {
            if (light.type == "ambient")
                back += light.intensity;
            else
            {
                if (light.type == "point")
                {
                    glm::vec3 lightDir = glm::normalize(light.position - hitPoint);
                    float lightDistance = glm::length(light.position - hitPoint);
                    
                    glm::vec3 shadowOrig = glm::dot(lightDir, normal) < 0 ? hitPoint - normal * 1e-3f : hitPoint + normal * 1e-3f;
                    glm::vec3 shadowDir = glm::normalize(light.position - hitPoint);
                    glm::vec3 shadowPt, shadowN;
                    Material tmpMaterial;
                    Sphere tmpSphere;

                    auto sceneIntersect = SceneIntersect(Ray(shadowOrig, shadowDir),scene.spheres, shadowPt,
                        shadowN, tmpMaterial,tmpSphere);

                    if (!sceneIntersect || tmpSphere.type == "lightSpere" || glm::length(shadowPt - shadowOrig) > lightDistance)
                    {
                        float nl = glm::dot(normal, lightDir);
                        diffuse += std::max(0.0f, nl) * light.intensity;

                        glm::vec3 r = glm::normalize(reflect(lightDir, normal));
                        float rv = glm::dot(v, r);
                        specular += light.intensity * glm::pow(std::max(0.0f, rv), specularExp);
                    }
                }
            }
        });
}

glm::vec3 Trace(const Ray& ray, Scene& scene, int depth = 0)
{
    glm::vec3 point, normal;
    Material material;
    Sphere closetSphere;

    if (depth > 4 || !SceneIntersect(ray, scene.spheres, point, normal, material,closetSphere))
    {
        return kDefaultBackgroundColor;
    }

    float kr = 1;
    fresnel(ray.direction, normal, material.albedo[3], kr);
    bool outside = glm::dot(normal, ray.direction) < 0;
    
    glm::vec3 reflectDir = glm::normalize(-reflect(ray.direction, normal));
    glm::vec3 reflectOrigin = glm::dot(reflectDir, normal) < 0 ? point - normal * 1e-2f : point + normal * 1e-2f;
    glm::vec3 reflectedColor = Trace(Ray(reflectOrigin, reflectDir), scene, depth + 1);

    float diffuse = 0, specular = 0, back = 0;
    Lighting(scene, normal, point, -ray.direction, material.specularExponent, diffuse, specular, back);
    
    glm::vec3 returnedColor =  material.color * back + material.color * diffuse * material.albedo[0] +
        glm::vec3(1.0f,1.0f,1.0f) * specular * material.albedo[1] + reflectedColor * material.albedo[2];
    returnedColor.r = std::min(1.0f, returnedColor.r);
    returnedColor.g = std::min(1.0f, returnedColor.g);
    returnedColor.b = std::min(1.0f, returnedColor.b);
    return returnedColor;
}

void render(Scene& scene)
{
    const int width = 4000;
    const int height = 2000;
    constexpr auto fov = glm::pi<float>() / 2;
    std::vector<glm::vec3> framebuffer(width * height);
    float imageAspectRatio = width / (float)height;

    for (size_t j = 0; j < height; j++)
    {
        for (size_t i = 0; i < width; i++) {
            float Px = (2 * (i + 0.5f) / (float)width - 1) * std::tanf(fov / 2.0f) * imageAspectRatio;
            float Py = (1 - 2 * (j + 0.5) / (float)height) * std::tanf(fov / 2.0f);
            glm::vec3 rayDirection = glm::normalize(glm::vec3(Px, Py, -1));
            framebuffer[i + j * width] = Trace(Ray(glm::vec3(0, 0, 0), rayDirection), scene);
        }
    }


    std::vector<unsigned char> imageData;
    for (auto&& elem : framebuffer)
    {
        imageData.push_back((unsigned char)(255 * elem.r));
        imageData.push_back((unsigned char)(255 * elem.g));
        imageData.push_back((unsigned char)(255 * elem.b));
    }

    stbi_write_jpg("out.jpg", width, height, 3, imageData.data(), 100);
}

int main()
{
    Material ivory(glm::vec3(0.4f, 0.4f, 0.3f), 50.0f,glm::vec4(0.6, 0.3, 0.1, 0.0));
    Material redRubber(glm::vec3(0.3f, 0.1f, 0.1f), 10.0f, glm::vec4(0.9, 0.1, 0.0, 0.0));
    Material rock(glm::vec3(0.5f, 0.48f, 0.48f), 10.0f,glm::vec4(0.9, 0.1, 0.0, 0.0), true);
    Material mirror(glm::vec3(0.84f, 0.61f, 0.61f), 125.0f, glm::vec4(0.0, 10.0, 0.8, 0.0));
    Material glass( glm::vec3(0.0f, 0.5f, 0.5f), 125.0f,glm::vec4(0.0, 0.5, 0.1, 0.8));
    Material light(glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, glm::vec4(1.0f,0.0f,0.0f,0.0f));

    Scene mainScene;
    mainScene.spheres.push_back(Sphere(glm::vec3(-3, 0 ,-15), 2, glass));
    mainScene.spheres.push_back(Sphere(glm::vec3(-1.0f,-1.5f, -12), 2, rock));
    mainScene.spheres.push_back(Sphere(glm::vec3(1.5, -0.5, -18), 3, redRubber));
    mainScene.spheres.push_back(Sphere(glm::vec3(7, 5, -18), 4, mirror));
    mainScene.spheres.push_back(Sphere(glm::vec3(-5.0f, 3.0f, -10.0f), 1, light,"lightSpere"));

    mainScene.lights.push_back(Light(glm::vec3(-20, 20, 20), 1.5f,"point"));
    mainScene.lights.push_back(Light(glm::vec3(30, 50, -25),0.5f,"point"));
    mainScene.lights.push_back(Light(glm::vec3(30, 20, 30), 0.6f,"point"));
    mainScene.lights.push_back(Light(glm::vec3(-10, 30, 30), 0.2f, "ambient"));

    render(mainScene);
}