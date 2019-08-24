#include <cmath>
#include <random>
#include <sstream>

#include <bmp/bmp.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <thinlens/camera/perspective.h>
#include <thinlens/auxiliaries/TestModel.h>
#include <thinlens/auxiliaries/utility.h>

using namespace std;
using glm::vec3;
using glm::mat3;

#define PI 3.141592653589793238462643383279502884

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES

/* Screen variables */
const int SCREEN_WIDTH = 480;
const int SCREEN_HEIGHT = 240;
bitmap_image image(SCREEN_WIDTH, SCREEN_HEIGHT);

/* Time */
int t;

/* Camera state */ 
float focalDistance = 0;
float lensRadius = 0;
float yaw = 0;
float pitch = 0;
vec3 cameraPos( 0, 0, -3 );

mat4 rotation;
mat3 R; // Y * P
mat3 Y; // Yaw rotation matrix (around y axis)
mat3 P; // Pitch rotation matrix (around x axis)

/* Model */
vector<Triangle> triangles;

/* Light source */
vec3 lightPos( 0, -0.5, -0.7 );
vec3 lightColor = 14.f * vec3( 1, 1, 1 );
vec3 indirectLight = 0.5f*vec3( 1, 1, 1 );

/* Path Tracing Parameters */
int maxDepth;
int numSamples;
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
    if(argc != 3){
        cerr << "Correct usage: " << argv[0] << " <max-depth> <num-samples>" << endl;
        return -1;
    }

    // read inputs

    stringstream ss;
    ss << argv[1] << " " << argv[2];
    ss >> maxDepth;

    if(!ss || maxDepth < 0){
        cerr << "first argument must be a positive integer" << endl;
        cerr << "Correct usage: " << argv[0] << " <max-depth> <num-samples>" << endl;
        return -1;
    }

    ss >> numSamples;

    if(!ss || numSamples < 0){
        cerr << "second argument must be a positive integer" << endl;
        cerr << "Correct usage: " << argv[0] << " <max-depth> <num-samples>" << endl;
        return -1;
    }

    if(!cin.eof()){
        if(!(cin >> focalDistance)){
            cerr << "incorrect format of read input" << endl;
            return -1;
        }
        if(!(cin >> lensRadius)){
            cerr << "incorrect format of read input" << endl;
            return -1;
        }
        if(!(cin >> cameraPos.x)){
            cerr << "incorrect format of read input" << endl;
            return -1;
        }
        if(!(cin >> cameraPos.y)){
            cerr << "incorrect format of read input" << endl;
            return -1;
        }
        if(!(cin >> cameraPos.z)){
            cerr << "incorrect format of read input" << endl;
            return -1;
        }
        if(!(cin >> pitch)){
            cerr << "incorrect format of read input" << endl;
            return -1;
        }
        if((cin >> yaw).fail()){
            cerr << "incorrect format of read input" << endl;
            return -1;
        }
    }

	srand(time(NULL));

	// load model
	LoadTestModel(triangles);

    Update();
	Draw();

	image.save_image("output.bmp" );
	return 0;
}

void Update()
{
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

    // scale a "basically unit" image plane according to raster ratio
	mat2 screenWindow = mat2();
    float ratio = float(SCREEN_WIDTH)/SCREEN_HEIGHT;

    screenWindow[0][0] = -ratio;
    screenWindow[0][1] = -1; // bottom left corner of window on image plane in screen space

    screenWindow[1][0] = 2*ratio;
    screenWindow[1][1] = 2; // width and height of window on image plane in screen space
	

	Camera *c = new PerspectiveCamera(cameraToWorld, screenWindow, 0, 10, lensRadius, focalDistance, 50, image);

	for(int i = 0; i < numSamples; ++i){
		
		cout << "Sample " << (i+1) << "/" << numSamples << endl; 
		
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
			}
		}
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
		return 0.7f*vec3(1,1,1);  // Nothing was hit; everything around you emits white light, e.g while outside
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