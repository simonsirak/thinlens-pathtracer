#include <projective.h>

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
                    screenToRaster = 
                        // [1] Screen -> NDC
                        // [2] NDC -> Raster
                        glm::scale( // [2]
                            glm::scale( // [1]
                                glm::translate( // [1]
                                    mat4(), 
                                    vec3(-screenWindow[0].x, -screenWindow[0].y, 0)
                                ),
                                vec3(1.f / screenWindow[1][0], -1.f / screenWindow[1][1], 1)
                            ),
                            vec3(film.width(), film.height(), 1)
                        );
                    
                        //std::cout << screenToRaster[3][3] << std::endl;

                    rasterToScreen = glm::inverse(screenToRaster);
                    rasterToCamera = glm::inverse(cameraToScreen) * rasterToScreen;
                }

