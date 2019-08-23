#ifndef PROJECTIVE_CAMERA_H
#define PROJECTIVE_CAMERA_H

#include <camera.h>
#include <bmp/bmp.h>
#include <glm/glm.hpp>

using glm::mat2;
using glm::mat4;

/*
    Screen space: All points have their xy coordinates mapped to the film plane.
    NDC (Normalized deivce coordinates): All points rescaled to fit unit cube.
    Raster space: 
*/
class ProjectiveCamera : public Camera {
public:
    ProjectiveCamera(const mat4 &cameraToWorld, 
               const mat4 &cameraToScreen, const mat2& screenWindow,
               float shutterOpen, float shutterClose, float lensr, float focald,
               const bitmap_image& film);
               
protected:
    mat4 cameraToScreen, rasterToCamera;
    mat4 screenToRaster, rasterToScreen;

public:
    float lensRadius, focalDistance;
};

#endif