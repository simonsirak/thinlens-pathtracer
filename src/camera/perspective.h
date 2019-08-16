#ifndef PERSPECTIVE_CAMERA_H
#define PERSPECTIVE_CAMERA_H

#include <camera.h>
#include <projective.h>

#include <bmp.h>
#include <glm/glm.hpp>

using glm::vec4;
using glm::mat2;
using glm::mat4;

/*
    Screen space: All points have their xy coordinates mapped to the film plane.
    NDC (Normalized deivce coordinates): All points rescaled to fit unit cube.
    Raster space: 
*/
class PerspectiveCamera : public ProjectiveCamera {
public:
    PerspectiveCamera(const mat4 &cameraToWorld, const mat2& screenWindow,
               float shutterOpen, float shutterClose, 
               float lensr, float focald, float fovy,
               const bitmap_image& film);

    // not necessary to declare in ProjectiveCamera, inheritance will still work;
    // should probably do it for clarity though
    float GenerateRay(const CameraSample& sample, Ray& ray) const override;
protected:
    vec4 dxCamera, dyCamera;
};

#endif