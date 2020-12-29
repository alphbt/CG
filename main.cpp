#include <glm.hpp>
#include <gtc/constants.hpp>
#include "scene.h"
#include "ray.h"
#include "material.h"
#include <algorithm>
#include <iostream>
#include "image.h"
#include <random>
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
        if (d > 0 && fabs(pt.x) < 30 && pt.z<2 && pt.z>-50 && d < spheres_dist) {
            checkerboard_dist = d;
            hit = pt;
            normal = glm::vec3(0, 1, 0);
            material.color = (int(0.5f * hit.x + 1000) + int(0.5f * hit.z)) & 1 ? glm::vec3(1.0f, 1.0f, 1.0f) 
               : glm::vec3(0.39f, 0.11f, 0.79f);
            material.color = material.color * 0.3f;
            material.albedo[2] = 0.1f;
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
                    glm::vec3 shadowPt1, shadowN1;
                    glm::vec3 shadowPt2, shadowN2;
                    glm::vec3 shadowPt3, shadowN3;
                    glm::vec3 shadowPt4, shadowN4;
                    Material tmpMaterial;
                    Sphere tmpSphere;

                    auto sceneIntersect = SceneIntersect(Ray(shadowOrig, shadowDir),scene.spheres, shadowPt,
                        shadowN, tmpMaterial,tmpSphere);
                    auto sceneIntersect1 = SceneIntersect(Ray(shadowOrig,glm::normalize(shadowDir + glm::vec3(0.01f))), scene.spheres, shadowPt1,
                        shadowN1, tmpMaterial, tmpSphere);
                    auto sceneIntersect2 = SceneIntersect(Ray(shadowOrig, glm::normalize(shadowDir - glm::vec3(0.01f))), scene.spheres, shadowPt2,
                        shadowN2, tmpMaterial, tmpSphere);
                    auto sceneIntersect3 = SceneIntersect(Ray(shadowOrig, glm::normalize(shadowDir + glm::vec3(0.02f))), scene.spheres, shadowPt3,
                        shadowN3, tmpMaterial, tmpSphere);
                    auto sceneIntersect4 = SceneIntersect(Ray(shadowOrig, glm::normalize(shadowDir - glm::vec3(0.02f))), scene.spheres, shadowPt4,
                        shadowN4, tmpMaterial, tmpSphere);

                    auto intersect = sceneIntersect || sceneIntersect1 || sceneIntersect2 || sceneIntersect3 || sceneIntersect4;
                    shadowPt = (shadowPt + shadowPt1 + shadowPt2 + shadowPt3 + shadowPt4) / 5.0f;

                    if (!intersect || tmpSphere.type == "lightSpere" || glm::length(shadowPt - shadowOrig) > lightDistance)
                    {
                        float nl = glm::dot(normal, lightDir);
                        diffuse += std::max(0.0f, nl) * light.intensity;

                        glm::vec3 h = glm::normalize(lightDir + v);
                        float nh = glm::dot(normal, h);
                        float m =1.0f;
                        float nv = glm::dot(normal, v);
                        float sigma = glm::pow(0.3, 2.0f);
                        float d = sigma / (glm::pi<float>() * glm::pow(nh * nh * (sigma - 1) + 1, 2.0f));
                        float f = glm::clamp(std::fabs(nv), 0.1f, 0.9f);;
                        float k = 2 * nh / glm::dot(h, v);
                        float g = std::min(1.0f, std::min(k * nv, k * nl));
                        float ct = d * f * g / (glm::pi<float>() * nv * nl);

                        specular += light.intensity * std::max(0.0f, ct);
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

    if (depth > 3 || !SceneIntersect(ray, scene.spheres, point, normal, material, closetSphere))
    {
        return kDefaultBackgroundColor;
    }

    if (closetSphere.type == "lightSpere")
        return material.color;
    
    bool outside = glm::dot(normal, ray.direction) < 0;
    
    glm::vec3 returnedColor(0);
    glm::vec3 reflectedColor(0);
    if (material.albedo[2] > 0.0f)
    {
        glm::vec3 reflectDir = glm::normalize(-reflect(ray.direction, normal));
        glm::vec3 reflectDir1 = glm::normalize(-reflect(ray.direction, glm::normalize(normal + glm::vec3(0.01f))));
        glm::vec3 reflectDir2 = glm::normalize(-reflect(ray.direction, glm::normalize(normal + glm::vec3(0.02f))));
        glm::vec3 reflectDir3 = glm::normalize(-reflect(ray.direction, glm::normalize(normal - glm::vec3(0.01f))));
        glm::vec3 reflectDir4 = glm::normalize(-reflect(ray.direction, glm::normalize(normal - glm::vec3(0.02f))));
        glm::vec3 reflectDir5 = glm::normalize(-reflect(ray.direction, glm::normalize(normal + glm::vec3(0.001f))));
        glm::vec3 reflectDir6 = glm::normalize(-reflect(ray.direction, glm::normalize(normal - glm::vec3(0.001f))));
        glm::vec3 reflectOrigin = outside < 0 ? point - normal * 1e-2f : point + normal * 1e-2f;
        reflectedColor = (Trace(Ray(reflectOrigin, reflectDir), scene, depth + 1) +
            Trace(Ray(reflectOrigin, reflectDir1), scene, depth + 1) +
            Trace(Ray(reflectOrigin, reflectDir2), scene, depth + 1) +
            Trace(Ray(reflectOrigin, reflectDir3), scene, depth + 1) +
            Trace(Ray(reflectOrigin, reflectDir4), scene, depth + 1) +
            Trace(Ray(reflectOrigin, reflectDir5), scene, depth + 1) +
            Trace(Ray(reflectOrigin, reflectDir6), scene, depth + 1)) / 7.0f;
    }

    float diffuse = 0, specular = 0, back = 0;
    Lighting(scene, normal, point, -ray.direction, material.specularExponent, diffuse, specular, back);
    
    returnedColor = material.color * back + material.color * diffuse * material.albedo[0] +
        glm::vec3(0.7f, 0.7f, 0.0f) * specular * material.albedo[1] + reflectedColor * material.albedo[2];
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
    Material mirror(glm::vec3(0.84f, 0.3f, 0.61f), 125.0f, glm::vec4(0.0, 0.9, 0.8, 0.0));
    Material light(glm::vec3(0.9f, 0.9f, 0.9f), 0.0f, glm::vec4(1.0f,0.0f,0.0f,0.0f));

    Scene mainScene;
    mainScene.spheres.push_back(Sphere(glm::vec3(-3, 0 ,-15), 2, ivory));
    mainScene.spheres.push_back(Sphere(glm::vec3(-1.0f,-1.5f, -12), 2, rock));
    mainScene.spheres.push_back(Sphere(glm::vec3(1.5, -0.5, -18), 3, redRubber));
    mainScene.spheres.push_back(Sphere(glm::vec3(7, 5, -18), 4, mirror));
    mainScene.spheres.push_back(Sphere(glm::vec3(-5.0f, 7.0f, -10.0f),0.5f, light,"lightSpere"));

    //mainScene.lights.push_back(Light(glm::vec3(-20, 20, 20), 1.5f,"point"));
    mainScene.lights.push_back(Light(glm::vec3(30, 50, -25),0.7f,"point"));
    mainScene.lights.push_back(Light(glm::vec3(30, 20, 30), 0.3f,"point"));

    glm::vec3 center(-5.0f, 7.0f, -10.0f);

    mainScene.lights.push_back(Light(glm::vec3(sqrt(3) / 12, sqrt(3) / 12, sqrt(3) / 12) + center, 0.1f, "point"));
    mainScene.lights.push_back(Light(glm::vec3(sqrt(3) / 12, sqrt(3) / 12,-sqrt(3) / 12) + center, 0.1f, "point"));
    mainScene.lights.push_back(Light(glm::vec3(-sqrt(3) / 12, sqrt(3) / 12,-sqrt(3) / 12) + center, 0.1f, "point"));
    mainScene.lights.push_back(Light(glm::vec3(-sqrt(3) / 12,sqrt(3) / 12, sqrt(3) / 12) + center, 0.1f, "point"));
    mainScene.lights.push_back(Light(glm::vec3(-sqrt(3) / 12,-sqrt(3) / 12,-sqrt(3) / 12) + center , 0.1f, "point"));
    mainScene.lights.push_back(Light(glm::vec3(-sqrt(3) / 12,-sqrt(3) / 12, sqrt(3) / 12) + center, 0.1f, "point"));
    mainScene.lights.push_back(Light(glm::vec3(sqrt(3) / 12,-sqrt(3) / 12, sqrt(3) / 12) + center, 0.1f, "point"));
    mainScene.lights.push_back(Light(glm::vec3(sqrt(3) / 12,-sqrt(3) / 12,-sqrt(3) / 12) + center, 0.1f, "point"));
    mainScene.lights.push_back(Light(center, 0.1f, "point"));


    mainScene.lights.push_back(Light(glm::vec3(-10, 30, 30), 0.2f, "ambient"));

    render(mainScene);
}