#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

// -------------------------------------------------
// Global Variables
// -------------------------------------------------
int Width = 480;
int Height = 480;
std::vector<float> OutputImage;

class Ray {
public:
	vec3 origin;
	vec3 direction;
	Ray(const vec3& o, const vec3& d) : origin(o), direction(normalize(d)) {}
};

class Camera {
public:
	vec3 eye;
	float l, r, b, t, d;
	Camera(vec3 e, float left, float right, float bottom, float top, float depth)
		: eye(e), l(left), r(right), b(bottom), t(top), d(depth) {}

    Ray generateRay(int i, int j, int width, int height, float u_offset = 0.5f, float v_offset = 0.5f) const {
        float u = l + (r - l) * (i + u_offset) / width;
        float v = b + (t - b) * (j + v_offset) / height;
        return Ray(eye, vec3(u, v, -d));
    }
};

struct Material {
    vec3 ka, kd, ks;
    float specular_power;
};

struct HitInfo {
    float t;
    vec3 point;
    vec3 normal;
    Material material;
};

class Surface {
public:
    Material material;
    virtual bool intersect(const Ray& ray, HitInfo& hit) const = 0;
};

class Sphere : public Surface {
public:
    vec3 center;
    float radius;
    Sphere(const vec3& c, float r, const Material& m) : center(c), radius(r) { material = m; }

    bool intersect(const Ray& ray, HitInfo& hit) const override {
        vec3 oc = ray.origin - center;
        float a = dot(ray.direction, ray.direction);
        float b = 2.0f * dot(oc, ray.direction);
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;
        if (discriminant < 0) return false;
        float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
        if (t < 0) t = (-b + std::sqrt(discriminant)) / (2.0f * a);
        if (t < 0) return false;
        hit.t = t;
        hit.point = ray.origin + t * ray.direction;
        hit.normal = normalize(hit.point - center);
        hit.material = material;
        return true;
    }
};

class Plane : public Surface {
public:
    float y;
    Plane(float yLevel, const Material& m) : y(yLevel) { material = m; }

    bool intersect(const Ray& ray, HitInfo& hit) const override {
        if (ray.direction.y == 0) return false;
        float t = (y - ray.origin.y) / ray.direction.y;
        if (t < 0) return false;
        hit.t = t;
        hit.point = ray.origin + t * ray.direction;
        hit.normal = vec3(0, 1, 0);
        hit.material = material;
        return true;
    }
};

class Scene {
public:
    std::vector<Surface*> objects;
    void addObject(Surface* obj) {
        objects.push_back(obj);
    }
    bool intersect(const Ray& ray, HitInfo& closestHit) const {
        bool hitSomething = false;
        float minT = 1e30f;
        for (const auto& obj : objects) {
            HitInfo hit;
            if (obj->intersect(ray, hit) && hit.t < minT) {
                minT = hit.t;
                closestHit = hit;
                hitSomething = true;
            }
        }
        return hitSomething;
    }
    bool isInShadow(const vec3& point, const vec3& lightPos) const {
        vec3 dir = lightPos - point;
        float distToLight = length(dir);
        Ray shadowRay(point + 1e-4f * normalize(dir), normalize(dir));
        HitInfo hit;
        if (intersect(shadowRay, hit) && hit.t < distToLight)
            return true;
        return false;
    }
};

vec3 phongShading(const HitInfo& hit, const vec3& lightPos, const vec3& eyePos, const Scene& scene) {
    vec3 color = hit.material.ka;
    vec3 L = normalize(lightPos - hit.point);
    vec3 V = normalize(eyePos - hit.point);
    vec3 R = reflect(-L, hit.normal);

    if (!scene.isInShadow(hit.point, lightPos)) {
        color += hit.material.kd * std::max(dot(hit.normal, L), 0.0f);
        color += hit.material.ks * pow(std::max(dot(R, V), 0.0f), hit.material.specular_power);
    }
    return clamp(color, 0.0f, 1.0f);
}

// 수정된 render 함수
void render() {
    OutputImage.clear();
    Camera camera(vec3(0, 0, 0), -0.1f, 0.1f, -0.1f, 0.1f, 0.1f);
    Scene scene;
    vec3 lightPos(-4, 4, -3);

    // Material 정의
    Material planeMat = { vec3(0.2), vec3(1), vec3(0), 0 };
    Material redMat = { vec3(0.2, 0, 0), vec3(1, 0, 0), vec3(0), 0 };
    Material greenMat = { vec3(0, 0.2, 0), vec3(0, 0.5, 0), vec3(0.5), 32 };
    Material blueMat = { vec3(0, 0, 0.2), vec3(0, 0, 1), vec3(0), 0 };

    // 오브젝트 등록
    scene.addObject(new Sphere(vec3(-4, 0, -7), 1, redMat));
    scene.addObject(new Sphere(vec3(0, 0, -7), 2, greenMat));
    scene.addObject(new Sphere(vec3(4, 0, -7), 1, blueMat));
    scene.addObject(new Plane(-2, planeMat));

    int N = 64;
    for (int j = 0; j < Height; ++j) {
        for (int i = 0; i < Width; ++i) {
            vec3 accumulatedColor(0.0f);
            for (int s = 0; s < N; ++s) {
                float u_offset = static_cast<float>(rand()) / RAND_MAX;
                float v_offset = static_cast<float>(rand()) / RAND_MAX;
                Ray ray = camera.generateRay(i, j, Width, Height, u_offset, v_offset);
                HitInfo hit;
                vec3 color(0);
                if (scene.intersect(ray, hit)) {
                    color = phongShading(hit, lightPos, camera.eye, scene);
                }
                float gamma = 2.2f;
                accumulatedColor += pow(color, vec3(1.0f / gamma));
            }
            accumulatedColor /= static_cast<float>(N);
            OutputImage.push_back(accumulatedColor.r);
            OutputImage.push_back(accumulatedColor.g);
            OutputImage.push_back(accumulatedColor.b);
        }
    }
}



void resize_callback(GLFWwindow*, int nw, int nh) 
{
	//This is called in response to the window resizing.
	//The new width and height are passed in so we make 
	//any necessary changes:
	Width = nw;
	Height = nh;
	//Tell the viewport to use all of our screen estate
	glViewport(0, 0, nw, nh);

	//This is not necessary, we're just working in 2d so
	//why not let our spaces reflect it?
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, static_cast<double>(Width)
		, 0.0, static_cast<double>(Height)
		, 1.0, -1.0);

	//Reserve memory for our render so that we don't do 
	//excessive allocations and render the image
	OutputImage.reserve(Width * Height * 3);
	render();
}


int main(int argc, char* argv[])
{
	// -------------------------------------------------
	// Initialize Window
	// -------------------------------------------------

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//We have an opengl context now. Everything from here on out 
	//is just managing our window or opengl directly.

	//Tell the opengl state machine we don't want it to make 
	//any assumptions about how pixels are aligned in memory 
	//during transfers between host and device (like glDrawPixels(...) )
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	//We call our resize function once to set everything up initially
	//after registering it as a callback with glfw
	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, Width, Height);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// -------------------------------------------------------------
		//Rendering begins!
		glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
		//and ends.
		// -------------------------------------------------------------

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		//Close when the user hits 'q' or escape
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
