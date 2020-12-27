#include <glm.hpp>
#include <gtc/constants.hpp>
#include "scene.h"
#include "ray.h"
#include "material.h"
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
    return  N * glm::dot(N, I) * 2.0f - I;
}

glm::vec3 Refract(const glm::vec3& I, glm::vec3& N, float& refractiveIndex)
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

static void getSphereTextureCoordinats(const glm::vec3& p, float& u, float& v) {
    float r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    float phi = std::atan2(-p.z,p.x);
    u = (phi + glm::pi<float>()) / (2 * glm::pi<float>());
    float theta = glm::acos(-p.y / r);
    v = theta / glm::pi<float>();
}

bool SceneIntersect(const Ray& ray, const std::vector<Sphere>& spheres, glm::vec3& hit,
    glm::vec3& normal, Material& material)
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

glm::vec3 CastRay(const Ray& ray, Scene& scene, int depth = 0)
{
    glm::vec3 point, normal;
    Material material;

    if (depth > 4 || !SceneIntersect(ray, scene.spheres, point, normal, material))
    {
        return kDefaultBackgroundColor;
    }

    glm::vec3 reflectDir = glm::normalize(reflect(ray.direction, normal));
    glm::vec3 reflectOrig = glm::dot(reflectDir, normal) < 0 ? point - normal * 1e-3f : point + normal * 1e-3f;
    glm::vec3 reflectColor = CastRay(Ray(reflectOrig, reflectDir), scene, depth + 1);

    float diffuseLightIntensity = 0;
    float specularLightIntensity = 0;
    glm::vec3 v = glm::normalize(ray.origin - point);

    for (size_t i = 0; i < scene.lights.size(); i++)
    {
        glm::vec3 lightDir = glm::normalize(scene.lights[i].position - point);
        float lightDistance = glm::length(scene.lights[i].position - point);
        glm::vec3 h = glm::normalize(v + lightDir);

        glm::vec3 shadowOrig = glm::dot(lightDir, normal) < 0 ? point - normal * 1e-3f : point + normal * 1e-3f;
        glm::vec3 shadowDir = glm::normalize(scene.lights[i].position - point);
        glm::vec3 shadow_pt, shadow_N;
        Material tmpmaterial;

        auto sceneIntersect = SceneIntersect(Ray(shadowOrig, shadowDir), scene.spheres, shadow_pt,
            shadow_N, tmpmaterial);

        if (sceneIntersect && glm::length(shadow_pt - shadowOrig) < lightDistance)
           continue; 

        float nh = glm::dot(normal, h);
        float m = 1.0f;
        float nv = glm::dot(normal, v);
        float nl = glm::dot(normal, lightDir);
        float d = glm::pow(glm::e<float>(), -(1 - nh * nh) / (m * m * nh * nh)) / (m * m * glm::pow(nh, 4.0f));
        float f = glm::mix(pow(1.0 - nv, 5.0), 1.0, material.refractiveIndex);
        float k = 2 * nh / glm::dot(h, v);
        float g = std::min(1.0f, std::min(k * nv, k * nl));
        float ct = d * f * g / (glm::pi<float>() * nv * nl);

        diffuseLightIntensity += scene.lights[i].intensity * std::max(0.0f, nl);
        specularLightIntensity += scene.lights[i].intensity  * std::max(0.0f, ct);// *
           // glm::pow(std::max(0.0f, glm::dot(h, normal)), material.specularExponent);

    }

    glm::vec3 returnColor = material.color * diffuseLightIntensity + glm::vec3(1.0f, 1.0f, 1.0f) * specularLightIntensity * 0.3f +
        + reflectColor * (float)material.isReflect + material.color * 0.2f;
    returnColor.r = std::min(1.0f, returnColor.r);
    returnColor.g = std::min(1.0f, returnColor.g);
    returnColor.b = std::min(1.0f, returnColor.b);
    return returnColor;
}

void render(Scene& scene)
{
    static int countOfRender = 1;
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
            framebuffer[i + j * width] = CastRay(Ray(glm::vec3(0, 0, 0), rayDirection), scene);
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
    Material ivory(glm::vec3(0.4f, 0.4f, 0.3f), 50.0f, 1.0f);
    Material redRubber( glm::vec3(0.3f, 0.1f, 0.1f), 10.0f, 1.0f);
    Material rock(glm::vec3(0.5f, 0.48f, 0.48f), 10.0f, 1.0f, true);
    Material mirror(glm::vec3(1.0f, 1.0f, 1.0f), 142.0f, 1.0f,false,true);
    Material glass( glm::vec3(0.0f, 0.5f, 0.5f), 125.0f, 1.333f);

    Scene mainScene;
    mainScene.spheres.push_back(Sphere(glm::vec3(-3, 0 ,-15), 2, ivory));
    mainScene.spheres.push_back(Sphere(glm::vec3(-1.0f,-1.5f, -12), 2, mirror));
    mainScene.spheres.push_back(Sphere(glm::vec3(1.5, -0.5, -18), 3, redRubber));
    mainScene.spheres.push_back(Sphere(glm::vec3(7, 5, -18), 4, rock));

    mainScene.lights.push_back(Light(glm::vec3(-20, 20, 20), 1.5));
   // mainScene.lights.push_back(Light(glm::vec3(30, 50, -25), 1.8));
   // mainScene.lights.push_back(Light(glm::vec3(30, 20, 30), 1.7));

    render(mainScene);
}