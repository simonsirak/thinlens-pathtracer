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
const int SCREEN_WIDTH = 300;
const int SCREEN_HEIGHT = 300;
bitmap_image image(SCREEN_WIDTH, SCREEN_HEIGHT);
SDL_Surface* screen;

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

/* Model */
vector<Triangle> triangles;

/* Light source */
vec3 lightPos( 0, -0.5, -0.7 );
vec3 lightColor = 14.f * vec3( 1, 1, 1 );
vec3 indirectLight = 0.5f*vec3( 1, 1, 1 );

/* Path Tracing Parameters */
int maxDepth = 10;
int numSamples = 500;
vec3 buffer[SCREEN_WIDTH][SCREEN_HEIGHT];

// ----------------------------------------------------------------------------
// FUNCTIONS

void Update();
void Draw();
bool ClosestIntersection(
	vec3 start, 
	vec3 dir,
	const vector<Triangle>& triangles, 
	Intersection& closestIntersection 
);

vec3 TracePath(Ray &r, int depth);

int main( int argc, char* argv[] )
{
	srand(time(NULL));

	// load model
	LoadTestModel(triangles);

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

	Camera *c = new PerspectiveCamera(cameraToWorld, screenWindow, 0, 10, 0.001, 3.2, 50, image);

	for(int i = 0; i < numSamples; ++i){
		
		cout << "Sample " << (i+1) << "/" << numSamples << endl; 

		if(!NoQuitMessageSDL()){
			return;
		}
		
		if( SDL_MUSTLOCK(screen) )
			SDL_LockSurface(screen);

		for( int y=0; y<SCREEN_HEIGHT; ++y ){
			for( int x=0; x<SCREEN_WIDTH; ++x ){

				CameraSample sample;
				sample.pFilm = vec2(x + rand() / (float)RAND_MAX, y + rand() / (float)RAND_MAX);
				sample.time = 0;
				sample.pLens = vec2(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);

				Ray r;

				c->GenerateRay(sample, r);

				vec3 old = buffer[x][y];
				vec3 color = TracePath(r, 0);
				buffer[x][y] = (old * float(i) + color)/float(i+1);
				vec3 bmpColor = glm::clamp(255.f * buffer[x][y], 0, 255);

				// cout << "vec4(" << r.d.x << "," << r.d.y << "," << r.d.z << "," << r.d.w << ")" << endl;
				image.set_pixel(x, y, bmpColor.r, bmpColor.g, bmpColor.b);
				PutPixelSDL(screen, x, y, buffer[x][y]);
			}
		}

		if( SDL_MUSTLOCK(screen) )
			SDL_UnlockSurface(screen);

		SDL_UpdateRect( screen, 0, 0, 0, 0 );
	}
}

bool ClosestIntersection(
	vec3 start, 
	vec3 dir,
	const vector<Triangle>& triangles, 
	Intersection& closestIntersection 
){
	closestIntersection.distance = std::numeric_limits<float>::max();
	dir = glm::normalize(dir); // does not matter what you do here, the t aka x.x will adjust the length. No need to normalize

	for(int i = 0; i < triangles.size(); ++i){
		const Triangle & triangle = triangles[i];
		using glm::vec3;
		using glm::mat3;
		vec3 v0 = triangle.v0;
		vec3 v1 = triangle.v1; 
		vec3 v2 = triangle.v2;
		vec3 e1 = v1 - v0;
		vec3 e2 = v2 - v0;
		vec3 b = start - v0;
		mat3 A( -dir, e1, e2 );
		vec3 x = glm::inverse( A ) * b;

		if(x.x > 0 && x.y >= 0 && x.z >= 0 && x.y <= 1 && x.z <= 1 && x.y + x.z <= 1){

			vec3 end = start + x.x * dir;
			float distance = glm::length(x.x * dir);
			if(distance < closestIntersection.distance){
				closestIntersection = {end, distance, i}; // element-wise copy assignment
			}
		}
	}

	if(closestIntersection.distance == std::numeric_limits<float>::max()){
		return false;
	} else {
		return true;
	}
}

vec3 TracePath(Ray &r, int depth) {
	if (depth >= maxDepth) {
		return vec3(0,0,0);  // Bounced enough times.
	}

	Intersection i;
	if (!ClosestIntersection(vec3(r.o.x,r.o.y,r.o.z),vec3(r.d.x,r.d.y,r.d.z),triangles,i)) {
		return vec3(0,0,0);  // Nothing was hit.
	}

	Triangle& triangle = triangles[i.triangleIndex];
	vec3 emittance = triangle.emittance;

	// Pick a random direction from here and keep going.
	Ray newRay;
	newRay.o = vec4(i.position,1);

	// This is NOT a cosine-weighted distribution!
	newRay.d = vec4(uniformHemisphereSample(triangle.normal, 1), 0);

	// Probability of the newRay
	const float p = uniformHemisphereSamplePDF(1);

	// Compute the BRDF for this ray (assuming Lambertian reflection)
	float cos_theta = glm::dot(vec3(newRay.d.x,newRay.d.y,newRay.d.z), triangle.normal);
	vec3 BRDF = triangle.color / float(PI) ; // color == reflectance

	// Recursively trace reflected light sources.
	vec3 incoming = TracePath(newRay, depth + 1);

	// Apply the Rendering Equation here.
	return emittance + (BRDF * incoming * cos_theta / p);
}