# src/camera
This directory contains different camera models for the 
pathtracer.

## Ray Generation
Generating a ray actually starts from the end frame (*raster frame*), 
and is transformed to the *camera frame*. When we calculate a ray 
differential, we are therefore getting the "in-CG-world"-distances
that correspond to a pixel shift.
