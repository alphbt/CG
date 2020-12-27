#include <glm.hpp>
#include <gtc/constants.hpp>
#include "scene.h"
#include "ray.h"
#include "material.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static const float kInfinity = std::numeric_limits<float>::max();
static const float kEpsilon = 1e-8;
static const glm::vec3 kDefaultBackgroundColor = glm::vec3(0.235294, 0.67451, 0.843137);


glm::vec3 reflect(const glm::vec3& I, const glm::vec3& N)
{
    return I - N * glm::dot(N, I) * 2.0f;
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
            normal = glm::normalize(hit - spheres[i].center);
            material.copy(spheres[i].material);
        }
    }

    return spheres_dist < 1000;
}

glm::vec3 CastRay(const Ray& ray, Scene& scene)
{
    glm::vec3 point, normal;
    Material material;

    if (!SceneIntersect(ray, scene.spheres, point, normal, material))
    {
        return kDefaultBackgroundColor;
    }

    float diffuse = 0;
    float specular = 0;
    glm::vec3 v = glm::normalize(ray.origin - point);

    for (size_t i = 0; i < scene.lights.size(); i++)
    {
        glm::vec3 lightDir = glm::normalize(scene.lights[i].position - point);
        diffuse += scene.lights[i].intensity * std::max(0.0f, glm::dot(lightDir,normal));
        specular += (float)scene.lights[i].intensity * 
            powf(std::max(0.0f, glm::dot(reflect(-v, normal), lightDir)), material.specularExponent);
    }

    return material.color * diffuse  +glm::vec3(1.0f, 1.0f, 1.0f) * specular;
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
    Material mirror(glm::vec3(0.0f, 0.0f, 0.9f), 142.0f, 1.0f);
    Material glass( glm::vec3(0.0f, 0.5f, 0.5f), 125.0f, 1.333f);

    Scene mainScene;
    mainScene.spheres.push_back(Sphere(glm::vec3(-3, 0 ,-15), 2, ivory));
    mainScene.spheres.push_back(Sphere(glm::vec3(-5, 3, -12), 2, redRubber));
    mainScene.spheres.push_back(Sphere(glm::vec3(1.5, -0.5, -18), 3, redRubber));
    mainScene.spheres.push_back(Sphere(glm::vec3(7, 5, -18), 4, ivory));

    mainScene.lights.push_back(Light(glm::vec3(-20, 20, 20), 1.5f));
    //mainScene.lights.push_back(Light(glm::vec3(30, 50, -25), 1.8f));
    //mainScene.lights.push_back(Light(glm::vec3(30, 20, 30), 1.7f));

    render(mainScene);
}