/*
CSCI 480
Assignment 3 Raytracer

Name: <Your name here>
*/

#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <stdio.h>
#include <string>
#include <iostream>

#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"

#include "assign3Math.h"

#define MAX_TRIANGLES 2000
#define MAX_SPHERES 10
#define MAX_LIGHTS 10

char *filename = 0;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2
int mode = MODE_DISPLAY;

//viewport dimensions
#define WIDTH 640
#define HEIGHT 480
#define ASPECT (double) WIDTH / HEIGHT

//the field of view of the camera
#define fov 60.0

#define PI 3.14159265
#define TORAD PI/180
#define RAYCUTOFF -100.0

//multisampling settings
#define NUMSAMPLESPERAXIS 2
#define MULTISAMPLING false

//attenuation settings
#define ATTENUATIONFACTOR 0.1
#define ATTENUATION true

unsigned char buffer[HEIGHT][WIDTH][3];

double w = ASPECT * tan(fov / 2 * TORAD);
double h = tan(fov / 2 * TORAD);
double xInc = 2 * w / WIDTH;
double yInc = 2 * h / HEIGHT;
point COF = { 0.0, 0.0, 0.0 };
double pixelOffset = 1.0 / (NUMSAMPLESPERAXIS + 1);

using namespace std;

struct Vertex
{
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double normal[3];
	double shininess;
};

typedef struct _Triangle
{
	struct Vertex v[3];
} Triangle;

typedef struct _Sphere
{
	double position[3];
	double color_diffuse[3];
	double color_specular[3];
	double shininess;
	double radius;
} Sphere;

typedef struct _Light
{
	double position[3];
	double color[3];
} Light;

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

vec triNorms[MAX_TRIANGLES];
double triAreas[MAX_TRIANGLES];
double triD[MAX_TRIANGLES];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
void plot_pixel(int x, int y, vec color);

void storeTriProperties() {
	for (int i = 0; i < num_triangles; ++i) {
		point A = triangles[i].v[0].position;
		point B = triangles[i].v[1].position;
		point C = triangles[i].v[2].position;
		triNorms[i] = cross(B - A, C - A).Normalize();
		triAreas[i] = 0.5 * cross(B - A, C - A).Length();
		triD[i] = -(triNorms[i].x * A.x + triNorms[i].y * A.y + triNorms[i].z * A.z);
	}
}

bool sphereIntersect(point center, double radius, point p0, vec direction, point& out) {
	double b = 2 * (dot(direction, p0 - center));
	double c = (p0 - center).LengthSq() - radius * radius;

	double t0 = (-b + sqrt(b * b - 4 * c)) / 2;
	double t1 = (-b - sqrt(b * b - 4 * c)) / 2;

	if (t0 > EPSILON || t1 > EPSILON) {
		out = p0 + direction * min(t0, t1);
		return true;
	}
	return false;
}

bool sphereLightBlocking(point center, double radius, point p0, vec direction) {
	double b = 2 * (dot(direction, p0 - center));
	double c = (p0 - center).LengthSq() - radius * radius;

	double t0 = (-b + sqrt(b * b - 4 * c)) / 2;
	double t1 = (-b - sqrt(b * b - 4 * c)) / 2;

	if (t0 > EPSILON || t1 > EPSILON) {
		return true;
	}
	return false;
}

bool triIntersect(int triIndex, point p0, vec direction, point& out, vec& baryCoords) {
	point A = triangles[triIndex].v[0].position;
	point B = triangles[triIndex].v[1].position;
	point C = triangles[triIndex].v[2].position;
	vec n = triNorms[triIndex];
	double d = triD[triIndex];
	vec vecP0(p0.x, p0.y, p0.z);
	double t = -(dot(n, vecP0) + d) / dot(n, direction);

	if (t > EPSILON) {
		out = p0 + direction * t;
		// Check winding order
		vec pA = out - A;
		vec pB = out - B;
		vec pC = out - C;
		if (dot(cross(pA, pB), cross(C - A, C - B)) > 0 && dot(cross(pB, pC), cross(A - B, A - C)) > 0 && dot(cross(pC, pA), cross(B - C, B - A)) > 0) {
			double totalArea = triAreas[triIndex];
			double weightA = 0.5 * cross(pB, pC).Length() / totalArea;
			double weightB = 0.5 * cross(pC, pA).Length() / totalArea;
			double weightC = 0.5 * cross(pA, pB).Length() / totalArea;
			baryCoords = { weightA, weightB, weightC };
			return true;
		}
	}
	return false;
}

bool triLightBlocking(int triIndex, point p0, vec direction) {
	point A = triangles[triIndex].v[0].position;
	point B = triangles[triIndex].v[1].position;
	point C = triangles[triIndex].v[2].position;
	vec n = triNorms[triIndex];
	double d = triD[triIndex];
	vec vecP0(p0.x, p0.y, p0.z);
	double t = -(dot(n, vecP0) + d) / dot(n, direction);

	if (t > EPSILON) {
		point intersect = p0 + direction * t;
		// Check winding order
		vec pA = intersect - A;
		vec pB = intersect - B;
		vec pC = intersect - C;
		if (dot(cross(pA, pB), cross(C - A, C - B)) > 0 && dot(cross(pB, pC), cross(A - B, A - C)) > 0 && dot(cross(pC, pA), cross(B - C, B - A)) > 0) {
			return true;
		}
	}
	return false;
}

vec sample(int horiPixel, int vertPixel, int sampleIndex) {
	vec direction;
	if (MULTISAMPLING) {
		// Same at points in the pixel
		// E.g., if NUMSAMPLES is 2, take samples at point (0.33, 0.33), (0.33, 0.66), (0.66, 0.33), (0.66, 0.66)
		point imagePos;
		imagePos.x = -w + (horiPixel + pixelOffset * (sampleIndex % NUMSAMPLESPERAXIS + 1)) * xInc;
		imagePos.y = -h + (vertPixel + pixelOffset * (sampleIndex / NUMSAMPLESPERAXIS + 1)) * yInc;
		imagePos.z = -1;
		direction = imagePos - COF;
	}
	else{
		direction = point(-w + (horiPixel + 0.5) * xInc, -h + (vertPixel + +0.5) * yInc, -1) - COF;
	}
	direction.Normalize();

	// Find closest intersection point with scene objects
	point closestIntersect = { 0.0, 0.0, RAYCUTOFF };
	vec n;
	vec colorDiffuse;
	vec colorSpecular;
	double shininess = 0.0;
	for (int i = 0; i < num_spheres; ++i) {
		Sphere sphere = spheres[i];
		point intersect;
		if (sphereIntersect(sphere.position, sphere.radius, COF, direction, intersect)) {
			if ((intersect - COF).LengthSq() < (closestIntersect - COF).LengthSq()) {
				closestIntersect = intersect;
				n = (1 / sphere.radius * (intersect - (point)sphere.position)).Normalize();
				colorDiffuse = sphere.color_diffuse;
				colorSpecular = sphere.color_specular;
				shininess = sphere.shininess;
			}
		}
	}
	for (int i = 0; i < num_triangles; ++i) {
		Vertex v1 = triangles[i].v[0];
		Vertex v2 = triangles[i].v[1];
		Vertex v3 = triangles[i].v[2];
		point intersect;
		vec baryCoords;
		if (triIntersect(i, COF, direction, intersect, baryCoords)) {
			if ((intersect - COF).LengthSq() < (closestIntersect - COF).LengthSq()) {
				closestIntersect = intersect;
				n = (vec)v1.normal * baryCoords.x + (vec)v2.normal * baryCoords.y + (vec)v3.normal * baryCoords.z;
				colorDiffuse = (vec)v1.color_diffuse * baryCoords.x + (vec)v2.color_diffuse * baryCoords.y + (vec)v3.color_diffuse * baryCoords.z;
				colorSpecular = (vec)v1.color_specular * baryCoords.x + (vec)v2.color_specular * baryCoords.y + (vec)v3.color_specular * baryCoords.z;
				shininess = v1.shininess * baryCoords.x + v2.shininess * baryCoords.y + v3.shininess * baryCoords.z;
			}
		}
	}

	// Calculate color
	if (closestIntersect.z != RAYCUTOFF) {
		vec color = ambient_light;
		for (int i = 0; i < num_lights; ++i) {
			Light light = lights[i];
			// Check if light is blocked
			vec shadowRay = (point)light.position - closestIntersect;
			shadowRay.Normalize();
			bool blocked = false;
			for (int i = 0; i < num_spheres; ++i) {
				Sphere sphere = spheres[i];
				if (sphereLightBlocking(sphere.position, sphere.radius, closestIntersect, shadowRay)) {
					blocked = true;
				}
			}
			for (int i = 0; i < num_triangles; ++i) {
				if (triLightBlocking(i, closestIntersect, shadowRay)) {
					blocked = true;
				}
			}

			// If not blocked, calculate the contribution of this light
			if (false == blocked) {
				vec l = ((point)light.position - closestIntersect).Normalize();
				vec r = (2 * (dot(l, n)) * n - l).Normalize();
				vec v = (COF - closestIntersect).Normalize();
				color.x += light.color[0] * (colorDiffuse.x * clamp(dot(l, n), 0.0, 1.0) + colorSpecular.x * pow(clamp(dot(r, v), 0.0, 1.0), shininess));
				color.y += light.color[1] * (colorDiffuse.y * clamp(dot(l, n), 0.0, 1.0) + colorSpecular.y * pow(clamp(dot(r, v), 0.0, 1.0), shininess));
				color.z += light.color[2] * (colorDiffuse.z * clamp(dot(l, n), 0.0, 1.0) + colorSpecular.z * pow(clamp(dot(r, v), 0.0, 1.0), shininess));
				color.x = clamp(color.x, 0.0, 1.0);
				color.y = clamp(color.y, 0.0, 1.0);
				color.z = clamp(color.z, 0.0, 1.0);

				// Attenuation
				if (ATTENUATION) {
					double distance = ((point)light.position - closestIntersect).Length();
					double attenuation = clamp(1.0 / ATTENUATIONFACTOR * distance, 0.0, 1.0);
					color *= attenuation;
				}
			}
		}

		

		return color;
	}
	return vec(1.0, 1.0, 1.0);
}

//MODIFY THIS FUNCTION
void draw_scene()
{
	storeTriProperties();
	glPointSize(2.0);
	// Loop through each pixel
	for (int a = 0; a < WIDTH; ++a) {
		for (int b = 0; b < HEIGHT; ++b) {
			vec color;
			if (MULTISAMPLING) {
				// Multisampling for antialiasing 
				for (int index = 0; index < NUMSAMPLESPERAXIS * NUMSAMPLESPERAXIS; ++index) {
					color += sample(a, b, index);
				}
				color = color / (NUMSAMPLESPERAXIS * NUMSAMPLESPERAXIS);
				color.x = clamp(color.x, 0.0, 1.0);
				color.y = clamp(color.y, 0.0, 1.0);
				color.z = clamp(color.z, 0.0, 1.0);
			}
			else {
				color = sample(a, b, 0);
			}
			
			// Draw pixel
			glBegin(GL_POINTS);
			plot_pixel(a, b, color);
			glEnd();
			glFlush();
		}
	}
	printf("Done!\n"); fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	glColor3f(((double)r) / 256.f, ((double)g) / 256.f, ((double)b) / 256.f);
	glVertex2i(x, y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	buffer[HEIGHT - y - 1][x][0] = r;
	buffer[HEIGHT - y - 1][x][1] = g;
	buffer[HEIGHT - y - 1][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
	plot_pixel_display(x, y, r, g, b);
	if (mode == MODE_JPEG)
		plot_pixel_jpeg(x, y, r, g, b);
}

void plot_pixel(int x, int y, vec color) {
	plot_pixel(x, y, color.x * 255, color.y * 255, color.z * 255);
}

/* Write a jpg image from buffer*/
void save_jpg()
{
	if (filename == NULL)
		return;

	// Allocate a picture buffer // 
	cv::Mat3b bufferBGR = cv::Mat::zeros(HEIGHT, WIDTH, CV_8UC3); //rows, cols, 3-channel 8-bit.
	printf("File to save to: %s\n", filename);

	// unsigned char buffer[HEIGHT][WIDTH][3];
	for (int r = 0; r < HEIGHT; r++) {
		for (int c = 0; c < WIDTH; c++) {
			for (int chan = 0; chan < 3; chan++) {
				unsigned char red = buffer[r][c][0];
				unsigned char green = buffer[r][c][1];
				unsigned char blue = buffer[r][c][2];
				bufferBGR.at<cv::Vec3b>(r, c) = cv::Vec3b(blue, green, red);
			}
		}
	}
	if (cv::imwrite(filename, bufferBGR)) {
		printf("File saved Successfully\n");
	}
	else {
		printf("Error in Saving\n");
	}
}

void parse_check(char *expected, char *found)
{
	if (stricmp(expected, found))
	{
		char error[100];
		printf("Expected '%s ' found '%s '\n", expected, found);
		printf("Parse error, abnormal abortion\n");
		exit(0);
	}

}

void parse_doubles(FILE*file, char *check, double p[3])
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check(check, str);
	fscanf(file, "%lf %lf %lf", &p[0], &p[1], &p[2]);
	printf("%s %lf %lf %lf\n", check, p[0], p[1], p[2]);
}

void parse_rad(FILE*file, double *r)
{
	char str[100];
	fscanf(file, "%s", str);
	parse_check("rad:", str);
	fscanf(file, "%lf", r);
	printf("rad: %f\n", *r);
}

void parse_shi(FILE*file, double *shi)
{
	char s[100];
	fscanf(file, "%s", s);
	parse_check("shi:", s);
	fscanf(file, "%lf", shi);
	printf("shi: %f\n", *shi);
}

int loadScene(char *argv)
{
	FILE *file = fopen(argv, "r");
	int number_of_objects;
	char type[50];
	int i;
	Triangle t;
	Sphere s;
	Light l;
	fscanf(file, "%i", &number_of_objects);

	printf("number of objects: %i\n", number_of_objects);
	char str[200];

	parse_doubles(file, "amb:", ambient_light);

	for (i = 0; i < number_of_objects; i++)
	{
		fscanf(file, "%s\n", type);
		printf("%s\n", type);
		if (stricmp(type, "triangle") == 0)
		{

			printf("found triangle\n");
			int j;

			for (j = 0; j < 3; j++)
			{
				parse_doubles(file, "pos:", t.v[j].position);
				parse_doubles(file, "nor:", t.v[j].normal);
				parse_doubles(file, "dif:", t.v[j].color_diffuse);
				parse_doubles(file, "spe:", t.v[j].color_specular);
				parse_shi(file, &t.v[j].shininess);
			}

			if (num_triangles == MAX_TRIANGLES)
			{
				printf("too many triangles, you should increase MAX_TRIANGLES!\n");
				exit(0);
			}
			triangles[num_triangles++] = t;
		}
		else if (stricmp(type, "sphere") == 0)
		{
			printf("found sphere\n");

			parse_doubles(file, "pos:", s.position);
			parse_rad(file, &s.radius);
			parse_doubles(file, "dif:", s.color_diffuse);
			parse_doubles(file, "spe:", s.color_specular);
			parse_shi(file, &s.shininess);

			if (num_spheres == MAX_SPHERES)
			{
				printf("too many spheres, you should increase MAX_SPHERES!\n");
				exit(0);
			}
			spheres[num_spheres++] = s;
		}
		else if (stricmp(type, "light") == 0)
		{
			printf("found light\n");
			parse_doubles(file, "pos:", l.position);
			parse_doubles(file, "col:", l.color);

			if (num_lights == MAX_LIGHTS)
			{
				printf("too many lights, you should increase MAX_LIGHTS!\n");
				exit(0);
			}
			lights[num_lights++] = l;
		}
		else
		{
			printf("unknown type in scene description:\n%s\n", type);
			exit(0);
		}
	}
	return 0;
}

void display()
{

}

void init()
{
	glMatrixMode(GL_PROJECTION);
	glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(1.0, 1.0, 1.0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
	//hack to make it only draw once
	static int once = 0;
	if (!once)
	{
		draw_scene();
		if (mode == MODE_JPEG)
			save_jpg();
	}
	once = 1;
}

int main(int argc, char ** argv)
{
	if (argc < 2 || argc > 3)
	{
		printf("usage: %s <scenefile> [jpegname]\n", argv[0]);
		exit(0);
	}
	if (argc == 3)
	{
		mode = MODE_JPEG;
		filename = argv[2];
	}
	else if (argc == 2)
		mode = MODE_DISPLAY;

	glutInit(&argc, argv);
	loadScene(argv[1]);

	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIDTH, HEIGHT);
	int window = glutCreateWindow("Ray Tracer");
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	init();
	glutMainLoop();
}
