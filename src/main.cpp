#include <bmp.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <cmath>
#include <random>
#include <perspective.h>
#include "TestModel.h"
#include <SDL.h>
#include "SDLauxiliary.h"

using namespace std;
using glm::vec3;
using glm::mat3;

#define PI 3.141592653589793238462643383279502884

// STRUCTS 
struct Intersection{
	vec3 position;
	float distance;
	int triangleIndex;
};

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

vec3 DirectLight( const Intersection& i );

int main( int argc, char* argv[] )
{
	srand(time(NULL));

	// load model
	LoadTestModel(triangles);

	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

	while( NoQuitMessageSDL() )
	{
		Update();
		Draw();
	}

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
	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);

	mat4 cameraToWorld = rotation;
	cameraToWorld[3] = vec4(cameraPos, 1);

	mat2 screenWindow = mat2();
	screenWindow[0][0] = -1;
	screenWindow[0][1] = -1; // bottom left corner of window on image plane in screen space

	screenWindow[1][0] = 2;
	screenWindow[1][1] = 2; // width and height of window on image plane in screen space

	Camera *c = new PerspectiveCamera(cameraToWorld, screenWindow, 0, 10, 0.01, 1, 50, image);
    for( int y=0; y<SCREEN_HEIGHT; ++y )
	{
		for( int x=0; x<SCREEN_WIDTH; ++x )
		{

			CameraSample sample;
			sample.pFilm = vec2(x + 0.5, y + 0.5);
			sample.time = 0;
			sample.pLens = vec2(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX);

			Ray r;

			c->GenerateRay(sample, r);

			// cout << "vec4(" << r.d.x << "," << r.d.y << "," << r.d.z << "," << r.d.w << ")" << endl;

			vec3 color( 0, 0, 0 );
			Intersection inter;
			if(ClosestIntersection(vec3(r.o.x, r.o.y, r.o.z), vec3(r.d.x, r.d.y, r.d.z), triangles, inter)){
                // This is the sequential form of division by numSamples.
                // connect() calculates a multi-sample estimator from 
                // the two paths using multiple importance sampling 
                // with the balance heuristic.

                vec3 color = glm::clamp(glm::clamp(255.f * DirectLight(inter), 0, 255.f) + 255.f * triangles[inter.triangleIndex].color * indirectLight, 0, 255);
                image.set_pixel(x, y, color.r, color.g, color.b);
				PutPixelSDL(screen, x, y, DirectLight(inter) + triangles[inter.triangleIndex].color * indirectLight);
			} else {
                image.set_pixel(x, y, color.r, color.g, color.b);
				PutPixelSDL(screen, x, y, vec3(0, 0, 0));
			}
		}
	}

	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
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

vec3 DirectLight( const Intersection& i ){
	
	// get radius from sphere defined by light position 
	// and the point of intersection.
	vec3 radius = (lightPos - i.position);
	float r2 = glm::length(radius)*glm::length(radius); // r^2
	radius = glm::normalize(radius); // normalize for future calculations

	vec3 light = lightColor/float(4.0f*PI*r2); // calculate irradiance of light

	/* 
		calculate normal in direction based on source of ray
		for the "first ray", it is the camera position that 
		is the source. However, for multiple bounces you would 
		need to embed the ray source into the Intersection 
		struct so that the correct normal can be calculated
		even when the camera is not the source.

		This makes the model only take the viewable surface into
		consideration. Note however that for any given ray source, only 
		one side of the surface is ever viewable , so this all works 
		out in the end. Multiple bounces would have different sources,
		so it is still possible that the other side of a surface can 
		receive light rays. Just not from light hitting the other side 
		of the surface.
	*/

	vec3 sourceToLight = cameraPos - i.position;
	vec3 normal = glm::dot(sourceToLight, triangles[i.triangleIndex].normal) * triangles[i.triangleIndex].normal / glm::dot(triangles[i.triangleIndex].normal, triangles[i.triangleIndex].normal);
	normal = glm::normalize(normal);

	/*
		Direction needs to either always or never be normalised. 
		Because it is not normalised in the draw function, I 
		will not normalize here.

		Also, I use a shadow bias (tiny offset in normal direction)
		to avoid "shadow acne" which is caused by self-intersection.
	*/

	Intersection blocker;
	if(ClosestIntersection(i.position + normal * 0.0001f, (lightPos - i.position), triangles, blocker) && glm::length(blocker.position - i.position) <= glm::length(lightPos-i.position)){
		return vec3(0, 0, 0);
	} else {
		return triangles[i.triangleIndex].color * light * (glm::dot(radius, normal) > 0.0f ? glm::dot(radius, normal) : 0.0f);
	}
}