#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>
#include <vector>
#include <limits>

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

// Ray class
class Ray {
public:
    vec3 origin;
    vec3 direction;

    Ray(const vec3& o, const vec3& d) : origin(o), direction(normalize(d)) {}
};

// Camera class
class Camera {
public:
    vec3 eye;
    vec3 u, v, w;
    float l, r, b, t, d;
    int nx, ny;

    Camera(const vec3& e, const vec3& u, const vec3& v, const vec3& w, float l, float r, float b, float t, float d, int nx, int ny)
        : eye(e), u(u), v(v), w(w), l(l), r(r), b(b), t(t), d(d), nx(nx), ny(ny) {
    }

    // Generate primary ray through pixel (i, j)
    Ray generateRay(int i, int j) const {
        float u_coord = l + (r - l) * (i + 0.5f) / nx;
        float v_coord = b + (t - b) * (j + 0.5f) / ny;
        vec3 dir = normalize(u * u_coord + v * v_coord - w * d);
        return Ray(eye, dir);
    }
};

// Material struct
struct Material {
    vec3 ka, kd, ks; // ambient, diffuse, specular
    float specPower; // specular power
};

// Abstract surface class
class Surface {
public:
    Material material;
    virtual ~Surface() = default;
    virtual bool intersect(const Ray& ray, float& t, vec3& normal) const = 0;
    virtual vec3 getPosition(const Ray& ray, float t) const = 0;
};

// Plane class
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

// Sphere class
class Sphere : public Surface {
public:
    vec3 center;
    float radius;

    Sphere(const vec3& center, float radius, const Material& mat) : center(center), radius(radius) {
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

// Scene class
class Scene {
public:
    std::vector<Surface*> objects;
    ~Scene() {
        for (auto obj : objects) delete obj;
    }

    void addObject(Surface* obj) {
        objects.push_back(obj);
    }

    bool intersect(const Ray& ray, vec3& hitColor, vec3& hitNormal, Material& mat, vec3& hitPos) const {
        float t_min = std::numeric_limits<float>::max();
        bool hit = false;
        for (auto& obj : objects) {
            float t;
            vec3 normal;
            if (obj->intersect(ray, t, normal) && t < t_min) {
                t_min = t;
                hitColor = obj->material.kd;
                mat = obj->material;
                hitNormal = normal;
                hitPos = obj->getPosition(ray, t);
                hit = true;
            }
        }
        return hit;
    }
};

// Rendering function
void render(const Camera& camera, const Scene& scene) {
    OutputImage.clear();
    vec3 lightPos = vec3(-4.0f, 4.0f, -3.0f);
    vec3 Ia = vec3(1.0f);  // ambient light
    vec3 Il = vec3(1.0f);  // point light
    float gamma = 2.2f;   // gamma correction

    for (int j = 0; j < Height; ++j) {
        for (int i = 0; i < Width; ++i) {
            Ray ray = camera.generateRay(i, j);
            vec3 hitColor, normal, hitPos;
            Material mat;
            vec3 pixelColor = vec3(0.0f);

            if (scene.intersect(ray, hitColor, normal, mat, hitPos)) {
                vec3 lightDir = normalize(lightPos - hitPos);
                Ray shadowRay(hitPos + 0.001f * lightDir, lightDir);
                vec3 tmpColor, tmpNormal, tmpPos;
                Material tmpMat;
                bool inShadow = scene.intersect(shadowRay, tmpColor, tmpNormal, tmpMat, tmpPos);

                vec3 V = normalize(-ray.direction);
                vec3 R = normalize(2.0f * dot(normal, lightDir) * normal - lightDir);

                vec3 ambient = mat.ka * Ia;
                vec3 diffuse(0.0f), specular(0.0f);
                if (!inShadow) {
                    diffuse = mat.kd * Il * std::max(dot(normal, lightDir), 0.0f);
                    specular = mat.ks * Il * pow(std::max(dot(R, V), 0.0f), mat.specPower);
                }

                pixelColor = ambient + diffuse + specular;
                pixelColor = clamp(pixelColor, 0.0f, 1.0f);

                // Apply gamma correction
                pixelColor.r = pow(pixelColor.r, 1.0f / gamma);
                pixelColor.g = pow(pixelColor.g, 1.0f / gamma);
                pixelColor.b = pow(pixelColor.b, 1.0f / gamma);
            }

            OutputImage.push_back(pixelColor.r);
            OutputImage.push_back(pixelColor.g);
            OutputImage.push_back(pixelColor.b);
        }
    }
}

void resize_callback(GLFWwindow*, int nw, int nh) {
    glViewport(0, 0, nw, nh);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, Width, 0.0, Height, 1.0, -1.0);
    OutputImage.reserve(Width * Height * 3);
}

int main(int argc, char* argv[]) {
    GLFWwindow* window;
    if (!glfwInit()) return -1;
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

    // Camera setup
    vec3 eye = vec3(0.0f);
    vec3 u = vec3(1.0f, 0.0f, 0.0f);
    vec3 v = vec3(0.0f, 1.0f, 0.0f);
    vec3 w = vec3(0.0f, 0.0f, 1.0f);
    float l = -0.1f, r = 0.1f, b = -0.1f, t = 0.1f, d = 0.1f;
    Camera camera(eye, u, v, w, l, r, b, t, d, Width, Height);

    // Materials
    Material planeMat = { vec3(0.2f), vec3(1.0f), vec3(0.0f), 0.0f };
    Material redMat = { vec3(0.2f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f), 0.0f };
    Material greenMat = { vec3(0.0f, 0.2f, 0.0f), vec3(0.0f, 0.5f, 0.0f), vec3(0.5f), 32.0f };
    Material blueMat = { vec3(0.0f, 0.0f, 0.2f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f), 0.0f };

    // Scene setup
    Scene scene;
    scene.addObject(new Plane(-2.0f, planeMat));
    scene.addObject(new Sphere(vec3(-4.0f, 0.0f, -7.0f), 1.0f, redMat));
    scene.addObject(new Sphere(vec3(0.0f, 0.0f, -7.0f), 2.0f, greenMat));
    scene.addObject(new Sphere(vec3(4.0f, 0.0f, -7.0f), 1.0f, blueMat));

    // Render scene
    render(camera, scene);

    // Display loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
        glfwSwapBuffers(window);
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}