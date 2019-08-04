#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <bmp.h>

using glm::mat4;
using glm::vec2;
using glm::vec3;

struct CameraSample {
    vec2 pFilm;
    vec2 pLens;
    float time;
};

struct Ray {
    vec3 o;
    vec3 d;
}

class Camera {    
public:
    mat4 cameraToWorld;
    float shutterOpen;
    float shutterClose;
    bitmap_image film;

    Camera(const mat4& cameraToWorld, 
            float shutterOpen, 
            float shutterClose, 
            bitmap_image film);

    /* 
        GenerateRay takes a given space-time CameraSample
        and generates a given ray from it. The resulting 
        ray is stored in the argument ray. 
        
        The function returns W_e, i.e the importance of 
        light reaching the film from the ray direction.
    */
    virtual float GenerateRay(const CameraSample& sample, Ray& ray) const = 0;

    /* TODO */
    //virtual float GenerateRayDifferential(const CameraSample& sample, RayDifferential& rd);

    virtual ~Camera();
}

#endif