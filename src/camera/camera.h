#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <bmp/bmp.h>

using glm::mat4;
using glm::vec2;
using glm::vec4;

struct CameraSample {
    vec2 pFilm;
    vec2 pLens;
    float time;
};

struct Ray {
    vec4 o;
    vec4 d;
    mutable float t; // distance along the direction, [0, infinity)
};

struct RayDifferential : public Ray {
    // initially, RayDifferentials have no differentials
    bool hasDifferentials;

    // origin and direction for differentials
    vec4 rxo;
    vec4 ryo;
    vec4 rxd;
    vec4 ryd;
};

class Camera {    
public:
    mat4 cameraToWorld;
    float shutterOpen;
    float shutterClose;
    const bitmap_image& film;

    Camera(const mat4& cameraToWorld, 
            float shutterOpen, 
            float shutterClose, 
            const bitmap_image& film);

    /* 
        GenerateRay takes a given space-time CameraSample
        and generates a given ray from it. The resulting 
        ray is stored in the argument ray. 
        
        The function returns W_e, i.e the importance of 
        light reaching the film from the ray direction.
    */
    virtual float GenerateRay(const CameraSample& sample, Ray& ray) const = 0;

    /* TODO */
    virtual float GenerateRayDifferential(const CameraSample& sample, RayDifferential& rd);

    virtual ~Camera();
};

#endif