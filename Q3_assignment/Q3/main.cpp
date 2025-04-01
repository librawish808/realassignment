#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#include <vector>
#include <limits>
#include <cmath>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// -------------------------------------------------
// Global Variables
// -------------------------------------------------
const int Width = 512;
const int Height = 512;
std::vector<float> OutputImage;
// -------------------------------------------------

class Ray {
public:
    vec3 origin;
    vec3 direction;

    Ray(const vec3& o, const vec3& d) : origin(o), direction(normalize(d)) {}
};

class Camera {
public:
    vec3 eye;
    vec3 u, v, w;
    float l, r, b, t, d;
    int nx, ny;

    Camera(const vec3& e, const vec3& u, const vec3& v, const vec3& w, float l, float r, float b, float t, float d, int nx, int ny)
        : eye(e), u(u), v(v), w(w), l(l), r(r), b(b), t(t), d(d), nx(nx), ny(ny) {
    }

    Ray generateRay(int i, int j) const {
        float u_coord = l + (r - l) * (i + 0.5f) / nx;
        float v_coord = b + (t - b) * (j + 0.5f) / ny;
        vec3 dir = normalize(u * u_coord + v * v_coord - w * d);
        return Ray(eye, dir);
    }
};


struct Material {
    vec3 ka, kd, ks;
    float specPower;
};
class Surface {
public:
    Material material;
    virtual ~Surface() = default;
    virtual bool intersect(const Ray& ray, float& t, vec3& normal) const = 0;
    virtual vec3 getPosition(const Ray& ray, float t) const = 0;
};


class Plane : public Surface {
public:
    float y;
    vec3 normal;

    Plane(float y, const Material& mat) : y(y), normal(vec3(0, 1, 0)) {
        material = mat;
    }

    bool intersect(const Ray& ray, float& t, vec3& outNormal) const override {
        if (ray.direction.y == 0) return false;
        t = (y - ray.origin.y) / ray.direction.y;
        if (t < 0) return false;
        outNormal = normal;
        return true;
    }

    vec3 getPosition(const Ray& ray, float t) const override {
        return ray.origin + t * ray.direction;
    }
};

class Sphere : public Surface {
public:
    vec3 center;
    float radius;

    Sphere(const vec3& c, float r, const Material& mat) : center(c), radius(r) {
        material = mat;
    }

    bool intersect(const Ray& ray, float& t, vec3& outNormal) const override {
        vec3 oc = ray.origin - center;
        float a = dot(ray.direction, ray.direction);
        float b = 2.0f * dot(oc, ray.direction);
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;

        if (discriminant < 0) return false;
        float temp = (-b - sqrtf(discriminant)) / (2.0f * a);
        if (temp < 0) return false;

        t = temp;
        vec3 hitPoint = ray.origin + t * ray.direction;
        outNormal = normalize(hitPoint - center);
        return true;
    }

    vec3 getPosition(const Ray& ray, float t) const override {
        return ray.origin + t * ray.direction;
    }
};

class Scene {
public:
    std::vector<Surface*> objects;

    ~Scene() {
        for (auto object : objects) {
            delete object;
        }
    }

    void addObject(Surface* object) {
        objects.push_back(object);
    }

    bool intersect(const Ray& ray, vec3& hitColor, vec3& hitNormal, Material& hitMaterial, vec3& hitPos) const {
        float t_min = std::numeric_limits<float>::max();
        bool hit = false;
        for (const auto& object : objects) {
            float t;
            vec3 normal;
            if (object->intersect(ray, t, normal) && t < t_min) {
                t_min = t;
                hitColor = object->material.kd;
                hitMaterial = object->material;
                hitNormal = normal;
                hitPos = object->getPosition(ray, t);
                hit = true;
            }
        }
        return hit;
    }
};

void render(const Camera& camera, const Scene& scene) {
    OutputImage.clear();
    vec3 lightPos = vec3(-4.0f, 4.0f, -3.0f);
    vec3 Ia = vec3(1.0f);  // ambient light
    vec3 Il = vec3(1.0f);  // light intensity

	int N = 64; // antialiasing sample count
    float gamma = 2.2f;

    for (int j = 0; j < Height; ++j) {
        for (int i = 0; i < Width; ++i) {
            vec3 accumulatedColor = vec3(0.0f);

            for (int s = 0; s < N; ++s) {
				//random offset
                float rx = static_cast<float>(rand()) / RAND_MAX;
                float ry = static_cast<float>(rand()) / RAND_MAX;

             
                Ray ray = camera.generateRay(i + rx - 0.5f, j + ry - 0.5f);

                vec3 hitColor, normal, hitPos;
                Material material;
                vec3 pixelColor = vec3(0.0f);

                if (scene.intersect(ray, hitColor, normal, material, hitPos)) {
                    // Cast a shadow ray toward the light source
                    vec3 lightDir = normalize(lightPos - hitPos);
                    Ray shadowRay(hitPos + 0.001f * lightDir, lightDir);
                    vec3 tempColor, tempNormal, tempHit;
                    Material tempMat;
                    bool inShadow = scene.intersect(shadowRay, tempColor, tempNormal, tempMat, tempHit);

                    vec3 V = normalize(-ray.direction);
                    vec3 R = normalize(2.0f * dot(normal, lightDir) * normal - lightDir);

                    // Shading calculation

                    vec3 ambient = material.ka * Ia;
                    vec3 diffuse = vec3(0.0f);
                    vec3 specular = vec3(0.0f);

                    if (!inShadow) {
                        diffuse = material.kd * Il * std::max(dot(normal, lightDir), 0.0f);
                        specular = material.ks * Il * pow(std::max(dot(R, V), 0.0f), material.specPower);
                    }

                    pixelColor = ambient + diffuse + specular;
                }

                // Clamp and apply gamma correction
                pixelColor = clamp(pixelColor, 0.0f, 1.0f);
                vec3 gammaCorrected = vec3(
                    pow(pixelColor.r, 1.0f / gamma),
                    pow(pixelColor.g, 1.0f / gamma),
                    pow(pixelColor.b, 1.0f / gamma)
                );

                accumulatedColor += gammaCorrected;
            }

            // Average the accumulated color over N samples
            vec3 finalColor = accumulatedColor / static_cast<float>(N);
            OutputImage.push_back(finalColor.r);
            OutputImage.push_back(finalColor.g);
            OutputImage.push_back(finalColor.b);
        }
    }
}

void resize_callback(GLFWwindow*, int nw, int nh) {
    glViewport(0, 0, nw, nh);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(Width), 0.0, static_cast<double>(Height), 1.0, -1.0);
    OutputImage.reserve(Width * Height * 3);
}

int main(int argc, char* argv[]) {
    GLFWwindow* window;
    if (!glfwInit())
        return -1;
    window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glfwSetFramebufferSizeCallback(window, resize_callback);

    vec3 eye = vec3(0.0f, 0.0f, 0.0f);
    vec3 u = vec3(1.0f, 0.0f, 0.0f);
    vec3 v = vec3(0.0f, 1.0f, 0.0f);
    vec3 w = vec3(0.0f, 0.0f, 1.0f);
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, d = 0.1f;
    int nx = 512, ny = 512;
    Camera camera(eye, u, v, w, l, r, b, t, d, nx, ny);

    Scene scene;
    Material planeMat = { vec3(0.2f), vec3(1.0f), vec3(0.0f), 0.0f };
    Material redMat = { vec3(0.2f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f), 0.0f };
    Material greenMat = { vec3(0.0f, 0.2f, 0.0f), vec3(0.0f, 0.5f, 0.0f), vec3(0.5f), 32.0f };
    Material blueMat = { vec3(0.0f, 0.0f, 0.2f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f), 0.0f };

	// objects
    scene.addObject(new Plane(-2.0f, planeMat));
    scene.addObject(new Sphere(vec3(-4.0f, 0.0f, -7.0f), 1.0f, redMat));
    scene.addObject(new Sphere(vec3(0.0f, 0.0f, -7.0f), 2.0f, greenMat));
    scene.addObject(new Sphere(vec3(4.0f, 0.0f, -7.0f), 1.0f, blueMat));

    
    
    
    render(camera, scene);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
        glfwSwapBuffers(window);
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}