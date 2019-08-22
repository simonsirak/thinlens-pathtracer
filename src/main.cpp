#include <bmp.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <cmath>
#include <random>
#include <perspective.h>
#include "TestModel.h"
#include "utility.h"
#include <SDL.h>
#include "SDLauxiliary.h"

using namespace std;
using glm::vec3;
using glm::mat3;

#define PI 3.141592653589793238462643383279502884

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

/* Screen variables */
const int SCREEN_WIDTH = 150;
const int SCREEN_HEIGHT = 150;
bitmap_image image(SCREEN_WIDTH, SCREEN_HEIGHT);
SDL_Surface* screen;

/*
	Meshes to be rendered are added to
	triangles. Lights to be used are
	added to triangles and lights.
*/
vector<Obj*> triangles;
vector<Obj*> lights;

/* BDPT parameters */
vec3 buffer[SCREEN_WIDTH][SCREEN_HEIGHT];
int numSamples = 200;
int maxDepth = 10;

/* Time */
int t;

/* Camera state */ 
float focalLength = SCREEN_HEIGHT;
float yaw = 0;
float pitch = 0;

/* Setters for the pitch and yaw given mouse coordinates relative to the center of screen */
#define PITCH(x, dt) (pitch += (SCREEN_WIDTH / 2.0f - x) * PI * 0.001f * dt / (SCREEN_WIDTH))
#define YAW(y, dt) (yaw += (y - SCREEN_HEIGHT / 2.0f) * PI * 0.001f * dt / (SCREEN_HEIGHT))

vec3 cameraPos( 0, 0, -3 );

mat4 rotation;
mat3 R; // Y * P
mat3 Y; // Yaw rotation matrix (around y axis)
mat3 P; // Pitch rotation matrix (around x axis)

/* Directions extracted from given mat3 */

#define FORWARD(R) (R[2])
#define RIGHT(R) (R[0])
#define UP(R) (R[1])

/* Light source */
vec3 lightPos( 0, -0.5, -0.7 );
vec3 lightColor = 14.f * vec3( 1, 1, 1 );
vec3 indirectLight = 0.5f*vec3( 1, 1, 1 );

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();
void Draw();
bool ClosestIntersection(
	vec4 start, 
	vec4 dir,
	const vector<Obj*>& triangles, 
	Intersection& closestIntersection 
);
int GenerateEyePath(int x, int y, vector<Vertex>& eyePath, int maxDepth, Camera* camera);
int GenerateLightPath(vector<Vertex>& lightPath, int maxDepth);
int TracePath(Ray r, vector<Vertex>& subPath, int maxDepth, bool isCameraPath, vec3 beta, float pdf);
vec3 connect(vector<Vertex>& lightPath, vector<Vertex>& eyePath);

int main( int argc, char* argv[] )
{
	srand(time(NULL));

	// load model
	LoadTestModel(triangles, lights);

	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

	// while( NoQuitMessageSDL() )
	// {
	// 	Update();
	// 	Draw();
	// }

	Draw();

	image.save_image("output.bmp" );
	return 0;
}

void Update()
{
	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;

	int x, y;
	if(SDL_GetMouseState(&x, &y) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		YAW(x, dt);
		PITCH(y, dt);
	}

	Uint8* keystate = SDL_GetKeyState( 0 );

	if( keystate[SDLK_UP] )
		lightPos += FORWARD(R) * 0.007f * dt;

	if( keystate[SDLK_DOWN] )
		lightPos -= FORWARD(R) * 0.007f * dt;

	if( keystate[SDLK_RIGHT] )
		lightPos += RIGHT(R) * 0.007f * dt;

	if( keystate[SDLK_LEFT] )
		lightPos -= RIGHT(R) * 0.007f * dt;

	if(keystate[SDLK_w]){
		cameraPos += FORWARD(R) * 0.007f * dt; // camera Z
	} 
	
	if(keystate[SDLK_s]){
		cameraPos -= FORWARD(R) * 0.007f * dt; // camera Z
	} 
	
	if(keystate[SDLK_a]){
		cameraPos -= RIGHT(R) * 0.007f * dt; // camera X
	} 
	
	if(keystate[SDLK_d]){
		cameraPos += RIGHT(R) * 0.007f * dt; // camera X
	} 
	
	if(keystate[SDLK_q]){
		cameraPos -= UP(R) * 0.007f * dt; // camera Y
	} 

	if(keystate[SDLK_e]){
		cameraPos += UP(R) * 0.007f * dt; // camera Y
	} 

	Y[0][0] = cos(yaw);
	Y[0][2] = -sin(yaw);
	Y[2][0] = sin(yaw);
	Y[2][2] = cos(yaw);

	P[1][1] = cos(pitch);
	P[1][2] = sin(pitch);
	P[2][1] = -sin(pitch);
	P[2][2] = cos(pitch);

	R = Y * P;
	rotation = mat4(R);
}

void Draw()
{
	mat4 cameraToWorld = rotation;
	cameraToWorld[3] = vec4(cameraPos, 1);

	mat2 screenWindow = mat2();
	screenWindow[0][0] = -1;
	screenWindow[0][1] = -1; // bottom left corner of window on image plane in screen space

	screenWindow[1][0] = 2;
	screenWindow[1][1] = 2; // width and height of window on image plane in screen space

	Camera *c = new PerspectiveCamera(cameraToWorld, screenWindow, 0, 10, 0.3, 2.3, 50, image);

	for(int i = 0 ; i < numSamples; ++i){
        if( SDL_MUSTLOCK(screen) )
            SDL_LockSurface(screen);

        cout << "Sample " << (i+1) << "/" << numSamples << endl; 

        for( int y=0; y<SCREEN_HEIGHT; ++y ){
            for( int x=0; x<SCREEN_WIDTH; ++x ){
                if(!NoQuitMessageSDL()){
					if( SDL_MUSTLOCK(screen) )
        				SDL_UnlockSurface(screen);
					return;
				}

                vector<Vertex> lightPath;
                vector<Vertex> eyePath;

                // Generate eye path
                GenerateEyePath(x, y, eyePath, maxDepth, c);

                // Generate light path
                GenerateLightPath(lightPath, maxDepth);

                vec3 old = buffer[x][y];

                // This is the sequential form of division by numSamples.
                // connect() calculates a multi-sample estimator from 
                // the two paths using multiple importance sampling 
                // with the balance heuristic.
                buffer[x][y] = (old * float(i) + connect(lightPath, eyePath))/float(i+1);

                PutPixelSDL( screen, x, y,  buffer[x][y]);
				vec3 color = glm::clamp(255.f * buffer[x][y], 0.f, 255.f);
				image.set_pixel(x, y, color.r, color.g, color.b);
            }
        }   

        if( SDL_MUSTLOCK(screen) )
        	SDL_UnlockSurface(screen);

        SDL_UpdateRect( screen, 0, 0, 0, 0 );
    }
}

bool ClosestIntersection(
	vec4 start, 
	vec4 dir,
	const vector<Obj*>& triangles, 
	Intersection& closestIntersection 
){
    closestIntersection.t = std::numeric_limits<float>::max();
    dir = glm::normalize(dir); // does not matter what you do here, the t aka x.x will adjust the length. No need to normalize
    Ray r = {start, dir, 0};

    int originalTriangle = closestIntersection.triangleIndex;

    for(int i = 0; i < triangles.size(); ++i){
        if(i == originalTriangle)
            continue;

        const Obj* triangle = triangles[i];
        double t = triangle->intersect(r);

        if(t > 0 && t < closestIntersection.t){ // 0.0001f is small epsilon to prevent self intersection
            closestIntersection.t = t;
            closestIntersection.triangleIndex = i;
        }
    }

    if(closestIntersection.t == std::numeric_limits<float>::max()){
        return false;
    } else {
        closestIntersection.position = r.o + r.d * closestIntersection.t;
        closestIntersection.normal = vec4(triangles[closestIntersection.triangleIndex]->normal(vec3(closestIntersection.position.x,closestIntersection.position.y,closestIntersection.position.z)), 0);
        return true;
    }
}

int GenerateEyePath(int x, int y, vector<Vertex>& eyePath, int maxDepth, Camera* camera){

    if(maxDepth == 0)
        return 0;

    vec3 normal(0, 0, 1);

	CameraSample sample;
	sample.pFilm = vec2(x + rand() / (float)RAND_MAX, y + rand() / (float)RAND_MAX);
	sample.time = 0;
	sample.pLens = vec2(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);

	RayDifferential r;
	
	vec3 beta = vec3(camera->GenerateRayDifferential(sample, r));
	float pdfPos, pdfDir;
	camera->Pdf_We(r, &pdfPos, &pdfDir);

    normal = glm::normalize(normal);
    r.d = glm::normalize(r.d);

    eyePath.push_back({1, 0, beta, -1, vec4(normal, 0), vec4(cameraPos, 1), r.d}); // the probability up to this point is 1

    return TracePath({r.o, r.d, 0}, eyePath, maxDepth - 1, true, beta, pdfDir) + 1;
}

int GenerateLightPath(vector<Vertex>& lightPath, int maxDepth){

    if(maxDepth == 0)
        return 0;

    // choose random light and find its index int triangles[],
    // the vector containing all shapes (yes I know it's a dumb
    // name since we have spheres now...)
    int index = rand() % lights.size();
    int triangleIndex = -1;

    Sphere * light = dynamic_cast<Sphere*>(lights[index]);   

    for(int i = 0; i < triangles.size(); ++i){
        if(light == triangles[i]){
            triangleIndex = i;
            break;
        }
    } 

    vec4 offset = uniformSphereSample(light->r);
    vec4 dir = uniformHemisphereSample(offset, 1);

    float lightChoiceProb = 1 / float(lights.size());
    float lightPosProb = uniformSphereSamplePDF(light->r);
    float lightDirProb = uniformHemisphereSamplePDF(1);
    float pointProb = lightChoiceProb * lightPosProb * lightDirProb;

    vec3 Le = vec3(light->emission, light->emission, light->emission);
    lightPath.push_back({lightChoiceProb * lightPosProb, 0, Le, triangleIndex, offset, vec4(light->c, 1) + offset, dir}); 

    /*			
        The light endpoint base case of the measurement 
        contribution function.

        Formally according to the measurement contribution 
        function, this calculation should be:

        = Le * G(current, next) / (lightChoiceProb * lightPosProb * lightDirProb * DirectionToAreaConversion(current, next))

        However certain factors in G and DirectionToAreaConversion
        require knowledge about the next point which we do not have.
        Luckily, these factors cancel out, and we only have to worry
        about the factor remaining in the calculation below.
    */

    vec3 beta = Le * glm::dot(glm::normalize(offset), dir) / pointProb;

    return TracePath({vec4(light->c, 1) + offset, dir, 0}, lightPath, maxDepth - 1, false, beta, uniformHemisphereSamplePDF(1)) + 1;
}

/*
    Generates a path from a certain starting 
    vertex (camera or light source) specified
    in the subPath[0]. The path generates at 
    most maxDepth additional vertices.

    The path starts off using the Ray r. The 
    base case sample contribution (from either) is passed 
    through from the beta vector.
*/
int TracePath(Ray r, vector<Vertex>& subPath, int maxDepth, bool isCameraPath, vec3 beta, float pdf) {

    if (maxDepth == 0) {
        return 0;  
    }

    int bounces = 0;
    float pdfFwd = pdf;
    float pdfRev = 0;

    while(true){
        Intersection point;
        point.triangleIndex = subPath[bounces].surfaceIndex; // avoid self intersection

        if (!ClosestIntersection(r.o, r.d, triangles, point)) { // Nothing was hit.
            break; 
        }

        if(++bounces >= maxDepth)
            break;

        Vertex vertex;
        Vertex *prev = &subPath[bounces-1];

        /* Process intersection data */
        Ray r1;

        r1.o = point.position;
        vec4 intersectionToOrigin = glm::normalize(r.o - r1.o);
        vec3 gnormal = glm::normalize(triangles[point.triangleIndex]->normal(vec3(r1.o.x, r1.o.y, r1.o.z)));
        vec4 snormal = glm::normalize(projectAOntoB(intersectionToOrigin, vec4(gnormal, 0))); // HAS to be normalized after projection onto another vector.
        r1.d = uniformHemisphereSample(snormal, 1); // regardless of which path, sample uniformly

        /* Construct vertex */
        vertex.position = point.position;
        vertex.normal = snormal;
        vertex.surfaceIndex = point.triangleIndex;

        // convert to area based density for by applying solidangle-to-area conversion
        vertex.pdfFwd = pdfFwd * DirectionToAreaConversion(*prev, vertex);

        // give currently constructed sample contribution to the 
        // current vertex
        vertex.c = beta;

        // add vertex to path
        subPath.push_back(vertex);

        // Refetch previous node in case of vector resizing
        prev = &subPath[bounces-1];

        // terminate if a light source was reached
        if(triangles[point.triangleIndex]->emission > 0){
            break;
        }

        // pdfFwd: Probability of sampling the next direction from current.
        // pdfRev: Probability of current sampling a direction towards previous. 
        // so pdfRev is calculated in reversed direction compared to direction of path generation.

        // Regardless of path, sample uniformly
        pdfFwd = uniformHemisphereSamplePDF(1);
        pdfRev = uniformHemisphereSamplePDF(1);

        prev->pdfRev = pdfRev * DirectionToAreaConversion(vertex, *prev);

        // append the contribution from the current intersection point 
        // onto the total sample contribution "beta"

        vec3 brdf = BRDF(vertex, r1.d, r.d, triangles, isCameraPath); 

        /*
            One of the many nested surface integral samples.

            Formally according to the measurement contribution 
            function, this calculation should be:

            *= (brdf * G(current, next) / (pdfFwd * DirectionToAreaConversion(current, next)))

            However certain factors in G and DirectionToAreaConversion
            require knowledge about the next point which we do not have.
            Luckily, these factors cancel out, and we only have to worry
            about the factor remaining in the calculation below.
        */

        // Using the old direction (r.d) is incorrect and 
        // will produce artefacts.
        beta *= (brdf * glm::abs(glm::dot(r1.d, snormal) / pdfFwd));

        // Don't forget to update the ray for the next iteration!
        r = r1;
    }

    return bounces;
}

/*
    Calculate multi-sample estimator using MIS. The light and 
    eye path are connected in different ways to form the many
    path sampling strategies specified in the report. The 
    contributions from these are then weighted with the balance 
    heuristic and added together, forming the multi-sample 
    estimator.
*/
vec3 connect(vector<Vertex>& lightPath, vector<Vertex>& eyePath){
    int t = lightPath.size();
    int s = eyePath.size();

    float basicScale = 1; //1 / (s + t + 2.);
    vec3 F;

    // t == 0, s >= 2:
    for(int i = 2; i <= s; ++i){
        Vertex &last = eyePath[i-1];
        if(triangles[last.surfaceIndex]->emission > 0){
            vec3 Le = vec3(triangles[last.surfaceIndex]->emission, triangles[last.surfaceIndex]->emission, triangles[last.surfaceIndex]->emission);
            F += Le * last.c * MIS(lightPath, eyePath, 0, i);
        }
    }

    // t >= 1, s >= 2:
    for(int i = 1; i <= t; ++i){
        for(int j = 2; j <= s; ++j){

            // perform visibility test that is related to the geometry 
            // term G. Only calculate contribution from this path 
            // sampling strategy if connection is possible.
            Intersection otherObj;
            otherObj.triangleIndex = lightPath[i-1].surfaceIndex; // previous vertex
            if(!ClosestIntersection(lightPath[i-1].position, (eyePath[j-1].position - lightPath[i-1].position), triangles, otherObj)){
                continue;
            } else {
                if(otherObj.triangleIndex != eyePath[j-1].surfaceIndex && otherObj.t > 0.01f){
                    continue;
                } else {
                
                /*
                    special case of light BRDF is to avoid 
                    index out of bounds. It essentially 
                    simulates "not using the light BRDF" if 
                    we are connecting directly to the light 
                    source. 

                    This is correct behavior, since there is 
                    no real meaning to the BRDF at the light 
                    source (There is no "incoming direction")
                */

                F += lightPath[i-1].c * eyePath[j-1].c 
                        * (i > 1 ? BRDF(lightPath[i-1], (lightPath[i-2].position - lightPath[i-1].position), (eyePath[j-1].position - lightPath[i-1].position), triangles, true) : vec3(1,1,1))
                        * G(lightPath[i-1], eyePath[j-1])
                        * BRDF(eyePath[j-1], (lightPath[i-1].position - eyePath[j-1].position), (eyePath[j-2].position - eyePath[j-1].position), triangles, true)
                        * triangles[eyePath[j-1].surfaceIndex]->color / float(PI)
                        * MIS(lightPath, eyePath, i, j);
                }
            } 
        }
    }

    // F is now the result of the multi-sample estimation, aka 
    // is a multi-sample estimator F. This is because we have taken 
    // a sample of a number of strategies defined by their pdf:s 
    // p(s,t). Note that the pdf division was baked into the 
    // calculation of the pre-computed sample contribution.

    return F; 
}