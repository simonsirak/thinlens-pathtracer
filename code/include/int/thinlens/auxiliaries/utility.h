#include <thinlens/auxiliaries/TestModel.h>
#include <glm/glm.hpp>
#include <cmath>
#include <random>

#define PI 3.141592653589793238462643383279502884

using glm::vec3;

/*
    More data structures.

    Intersection structure is largely the same as 
    Lab 2 of rendering track.

    Vertex structure is used for storing information
    about each vertex in a subpath.
*/
struct Intersection{
	vec3 position;
	float distance;
	int triangleIndex;
};

std::random_device rd;  //Will be used to obtain a seed for the random number engine

using namespace std;
using glm::vec3;
using glm::mat3;

/*
    Calculates and returns the orthogonal 
    projection of vector a onto vector b.
*/
vec3 projectAOntoB(const vec3 & a, const vec3 & b){
    vec3 c = glm::dot(a, b) * b / glm::dot(b, b);
    return c;
}

/*
    Returns a uniform sphere sample.
*/
vec3 uniformSphereSample(float r){

    std::uniform_real_distribution<float> dis(0, 1.0);

    float theta0 = 2*PI*dis(rd);
    float theta1 = acos(1 - 2*dis(rd));

    vec3 dir = vec3(sin(theta1)*sin(theta0), sin(theta1)*cos(theta0), cos(theta1)); 

    dir = r * glm::normalize(dir);

    return dir;
}

/*
    PDF for uniform sphere sample.
*/
float uniformSphereSamplePDF(float r){
    return 1/(r*r*4*PI);
}

/*
    Sample hemisphere uniformly around an axis.
*/
vec3 uniformHemisphereSample(const vec3 & axis, float r){

    std::uniform_real_distribution<float> dis(0, 1.0);

    float theta0 = 2*PI*dis(rd);
    float theta1 = acos(1 - 2*dis(rd));

    vec3 dir = vec3(sin(theta1)*sin(theta0), sin(theta1)*cos(theta0), cos(theta1)); 

    dir = glm::normalize(projectAOntoB(axis, dir));

    return dir;
}

/*
    Get PDF for a uniform hemisphere sample.
*/
float uniformHemisphereSamplePDF(float r){
    return 1 / (r*r*2*PI);
}