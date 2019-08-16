#include <perspective.h>

#include <bmp.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using glm::mat2;
using glm::mat4;
using glm::vec4;
using glm::vec3;

namespace {
    #define PI 3.141592653589793238462643383279502884
    mat4 Perspective(float fov, float n, float f) {
        mat4 persp;
        persp[3][3] = 0;
        persp[2][3] = 1;
        persp[2][2] = f / (f - n);
        persp[3][2] = -f * n / (f - n);

        float invTanAng = 1 / std::tan(fov * (PI / 180.f) / 2);
        return glm::scale(persp, vec3(invTanAng, invTanAng, 1));
    }
};

PerspectiveCamera::PerspectiveCamera(
    const mat4 &cameraToWorld, const mat2& screenWindow,
    float shutterOpen, float shutterClose, 
    float lensr, float focald, float fovy,
    const bitmap_image& film
): ProjectiveCamera(
    cameraToWorld, 
    Perspective(fovy, 1e-2f, 1000.f), 
    screenWindow, shutterOpen, shutterClose, lensr, focald, film) 
{
    vec4 x2 = rasterToCamera * vec4(1,0,0,1); x2 /= x2.w;
    vec4 x1 = rasterToCamera * vec4(0,0,0,1); x1 /= x1.w;
    vec4 y2 = rasterToCamera * vec4(0,1,0,1); y2 /= y2.w;
    vec4 y1 = rasterToCamera * vec4(0,0,0,1); y1 /= y1.w;
    dxCamera = x2 - x1; dxCamera.w = 0;
    dyCamera = y2 - y1; dyCamera.w = 0;
}

float PerspectiveCamera::GenerateRay(const CameraSample& sample, Ray& ray) const {
    vec4 pFilm = vec4(sample.pFilm.x, sample.pFilm.y, 0, 1);
    vec4 pCamera = rasterToCamera * pFilm; 
    //pCamera /= pCamera.w; // contains a projection (screen <-> camera)
    
    ray.o = vec4(0, 0, 0, 1);
    ray.d = glm::normalize(vec4(pCamera.x, pCamera.y, pCamera.z, 0));

    // contains no projection so no need for division ray.o /= ray.o.w;
    ray.o = cameraToWorld * ray.o; 

    //std::cout << cameraToWorld[0][0] << std::endl;
    // contains no projection so no need for modification ray.d.w = 0; ray.d = glm::normalize(ray.d);
    ray.d = cameraToWorld * ray.d;

    if(sample.pFilm.x == 0 || sample.pFilm.y == 0){
        std::cout << "pixel: " << glm::to_string(sample.pFilm) << std::endl;
        std::cout << "o: " << glm::to_string(ray.o) << std::endl;
        std::cout << "d: " << glm::to_string(ray.d) << std::endl;
    }

    //    std::cout << pCamera.x << " " << pCamera.y << " " << pCamera.z << " " << pCamera.w << std::endl;
    // transform ray from camera to world coordinates
    
    return 1;
}

