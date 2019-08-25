#include <thinlens/camera/projective.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

// screenWindow is a matrix [v1, v2] where
// v1 is top left point and v2 is width and height
// mat[x] gives column x of matrix mat. 
ProjectiveCamera::ProjectiveCamera(const mat4 &cameraToWorld, 
               const mat4 &cameraToScreen, const mat2& screenWindow,
               float shutterOpen, float shutterClose, float lensr, float focald,
               const bitmap_image& film): 
                Camera(cameraToWorld, shutterOpen, shutterClose, film),
                cameraToScreen(cameraToScreen),
                lensRadius(lensr), focalDistance(focald) {
                    // [1]
                    mat4 m1 = glm::translate(mat4(), vec3(-screenWindow[0].x, -screenWindow[0].y, 0));
                    mat4 m2 = glm::scale(mat4(), vec3(1.f / screenWindow[1][0], 1.f / screenWindow[1][1], 1));

                    // [2]
                    mat4 m3 = glm::scale(mat4(), vec3(film.width(), film.height(), 1));
                    screenToRaster = m3 * m2 * m1;

                    rasterToScreen = glm::inverse(screenToRaster);
                    rasterToCamera = glm::inverse(screenToRaster * cameraToScreen);
                    //std::cout << rasterToCamera[3][3] << std::endl;
                }

