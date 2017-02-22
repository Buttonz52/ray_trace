#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <stdio.h>

using namespace glm;
using namespace std;

struct Triangle
{
	vec3 P0;
	vec3 P1;
	vec3 P2;
	vec3 color;
};

struct Ray
{
	vec3 origin;
	vec3 direction;
};

struct Plane
{
	vec3 normal;
	vec3 position;
	vec3 color;
};

vec3 light;
vec3 camera;
vector<vec3> shape;
vector<Triangle> triangles;
vector<vec2> pixel_pos;
vector<vec3> colors;

float PI = 3.14159265;
int degree = 60;
float FoV = degree * PI / 180;

int main(int argc, char *argv[])
{
	unsigned int num_of_threads = 4; //number of threads
	unsigned char sharedBuffer[500000];

	cin >> sharedBuffer;	//pipe?

	getInfo(sharedBuffer); // assumes [header,scene,pixel_pos]

	for (int i = 0; i < pixel_pos.size(); i++)
	{
		vec3 c = ray_trace(pixel_pos[i].x, pixel_pos[i].y);
		colors.push_back(c);
	}

	cout << &colors[0];

	return 0;
}

void getInfo(unsigned char* buffer)
{
	int *header = (int*)&buffer[0];
	int *scene = (int*)&buffer[4];
	int *data = (int*)&buffer[8];
	float *lx = (float*)&buffer[12];
	float *ly = (float*)&buffer[16];
	float *lz = (float*)&buffer[20];
	float *cx = (float*)&buffer[24];
	float *cy = (float*)&buffer[28];
	float *cz = (float*)&buffer[32];

	light.x = *lx;
	light.y = *ly;
	light.z = *lz;

	camera.x = *cx;
	camera.y = *cy;
	camera.z = *cz;

	int length = (*scene - 24) / 4;
	int base = 36;

	for (int i = 0; i < length; i++)	//get scene and shape data
	{
		float *x = (float*)&buffer[base + (i * 4)];
		float *y = (float*)&buffer[base + (i + 1 * 4)];
		float *z = (float*)&buffer[base + (i + 2 * 4)];
		shape[i].x = *x;
		shape[i].y = *y;
		shape[i].z = *z;
	}

	getTriangles();	//convert shape to vector<triangle> triangles

	int length = (*data) / 4;
	int base = (*header) + (*scene);

	for (int i = 0; i < length; i++)
	{
		int *px = (int*)&buffer[base + (i * 4)];
		int *py = (int*)&buffer[base + (i + 1 * 4)];
		pixel_pos[i].x = *px;
		pixel_pos[i].y = *py;
	}
}

void getTriangles()
{
	for (int i = 0; i < shape.size(); i = i + 3)
	{
		Triangle t;
		t.P0 = shape[i];
		t.P1 = shape[i + 1];
		t.P2 = shape[i + 2];
		t.color = vec3(1.0, 0.0, 0.0); //red
		triangles.push_back(t);
	}

}

vec3 ray_trace(int i, int j)
{
	vec3 cameraOrigin(0, 0, 0);			//place camera origin
	vec3 color;
	int l, r, t, b;						//init and set 
	r = t = 1;
	l = b = -r;

	// call function to draw our scene

	Ray newRay; //init ray to be shot out of camera
	vec4 intersect; //init data vector for all intersects
	vec4 closestInteresectAndColor(1.0, 1.0, 1.0, numeric_limits<float>::max()); //return color if there exists an intersection
	bool doesIntersect = false; //start every ray as non-intersect

								//Calculates camera ray direction vector
	float u = l + ((r - l) * (i + 0.5)) / (200);
	float v = b + ((t - b) * (j + 0.5)) / (200);
	float w = -(r / tan(FoV / 2)); //dynamic Field of View

								   //Ray data assignment
	newRay.origin = cameraOrigin;
	newRay.direction = normalize(vec3(u, v, w) - cameraOrigin);

	//check intersect with all triangles
	for (int i = 0; i < triangles.size(); i++)
	{
		intersect = intersectTriangle(newRay, triangles.at(i), light, true);
		if (intersect.w != NULL)
			doesIntersect = true;
		if (intersect.w < closestInteresectAndColor.w && (intersect.w != NULL))
			closestInteresectAndColor = intersect;
	}

	if (doesIntersect == true) //only if there was an intersect this ray, draw pixel
	{
		color = vec3(closestInteresectAndColor);

	}
	else
	{
		color = vec3(0.0, 0.0, 0.0);
	}
	return color;
}


vec4 intersectTriangle(Ray r, Triangle tri, vec3 light, bool calcPong)
{
	//compute normal vector of plane which triangle resides
	vec3 P1P0 = tri.P1 - tri.P0;
	vec3 P2P0 = tri.P2 - tri.P0;
	vec3 P2P1 = tri.P2 - tri.P1;
	vec3 P0P2 = tri.P0 - tri.P2;
	vec3 normal = normalize(cross(P1P0, P2P0));
	float t;

	Plane p;
	p.normal = normal;
	p.position = tri.P0;

	//vec4 plane = intersectPlane(ray, p, light);

	float para = dot(normal, r.direction);
	//first calc point on plane
	if (para != 0)
	{
		vec3 ppco = p.position - r.origin; //planePosition -cameraOrigin
		float t = dot(ppco, normal) / para;

		if (t < 0)
			return vec4(NULL, NULL, NULL, NULL);

		vec3 x = r.origin + (t*r.direction); //plane.w is t

											 //then calc if point is on triangle
		float a = dot(cross(P1P0, (x - tri.P0)), normal);
		float b = dot(cross(P2P1, (x - tri.P1)), normal);
		float c = dot(cross(P0P2, (x - tri.P2)), normal);
		vec3 color;

		if (a >= -0.001 && b >= -0.001 && c >= -0.001)
		{
			if (calcPong == true)
			{
				vec3 color = Phong(light, x, normal, tri.color, r, false);
				return vec4(color, t);
			}
			else
			{
				return vec4(tri.color, t);
			}

		}
		else
		{
			return vec4(NULL, NULL, NULL, NULL);
		}
	}
	return vec4(NULL, NULL, NULL, NULL);
}

vec3 Phong(vec3 light, vec3 point, vec3 normal, vec3 color, Ray r, bool draw)
{
	vec3 l = normalize(light - point);
	vec3 v = normalize(r.origin - point);
	vec3 n = normalize(normal);

	vec3 h = normalize(v + l);

	vec3 ka = color;
	vec3 kd = ka;
	vec3 ks = vec3(0.7, 0.7, 0.7);
	float Ia = 0.2;
	float I = 1.0;
	int exp = 16.0;
	bool isShadow = false;

	float normalOffset = 0.1;

	vec3 ambient = ka * Ia;
	vec3 diffuse = kd * I * max((float)0.0, dot(n, l));
	vec3 specular = ((float)(I * (pow(max((float)0.0, dot(n, h)), exp)))) * ks;
	vec3 L = ambient + diffuse + specular;


	if (draw == false)
		return ambient + diffuse;
	else
		return L;
}