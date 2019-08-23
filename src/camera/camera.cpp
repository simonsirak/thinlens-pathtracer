#include <camera.h>

Camera::Camera(const mat4& cameraToWorld, 
            float shutterOpen, 
            float shutterClose, 
            const bitmap_image& film): cameraToWorld(cameraToWorld),
                                shutterOpen(shutterOpen),
                                shutterClose(shutterClose),
                                film(film) {}

float Camera::GenerateRayDifferential(const CameraSample& sample, RayDifferential& rd) {
    float wt = GenerateRay(sample, rd);
    CameraSample sshift = sample;
    sshift.pFilm.x++;
    Ray rx;
    float wtx = GenerateRay(sshift, rx);
    if (wtx == 0) return 0;
    rd.rxo = rx.o;
    rd.rxd = rx.d;

    sshift.pFilm.x--;
    sshift.pFilm.y++;
    Ray ry;
    float wty = GenerateRay(sshift, ry);
    if (wty == 0) return 0;
    rd.ryo = ry.o;
    rd.ryd = ry.d;

    rd.hasDifferentials = true;
    return wt; 
}

Camera::~Camera() {}