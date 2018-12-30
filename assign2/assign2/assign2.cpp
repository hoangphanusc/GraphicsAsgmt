// assign2.cpp : Defines the entry point for the console application.
//

/*
	CSCI 480 Computer Graphics
	Assignment 2: Simulating a Roller Coaster
	C++ starter code
*/

#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <iostream>
#include <GL/glu.h>
#include <GL/glut.h>
#include "assign2Math.h"

#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"

using namespace std;

int g_iMenuId;

/* Window variables */
const int _windowWidth = 640;
const int _windowHeight = 480;
const int pov = 100;

/* Input variables */
int g_vMousePos[2] = { 0, 0 };
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE g_ControlState = ROTATE;

/* State of the world */
float g_vLandRotate[3] = { 0.0, 0.0, 0.0 };
float g_vLandTranslate[3] = { 0.0, 0.0, 0.0 };
float g_vLandScale[3] = { 1.0, 1.0, 1.0 };

/* Textures */
GLuint textures[7];
cv::Mat3b groundTexture; const int groundID = 0;
cv::Mat3b woodTexture; const int woodID = 1;
cv::Mat3b skyUpTexture; const int skyUpID = 2;
cv::Mat3b sky1Texture; const int sky1ID = 3;
cv::Mat3b sky2Texture; const int sky2ID = 4;
cv::Mat3b sky3Texture; const int sky3ID = 5;
cv::Mat3b sky4Texture; const int sky4ID = 6;

/* Spline struct which contains how many control points, and an array of control points */
struct spline {
	int numControlPoints;
	struct point *points;
};

/* The spline array */
struct spline *g_Splines;

/* Total number of splines */
int g_iNumOfSplines;

/* Points on track */
vector<point> _ridePoints;
double trackMaxHeight = 0.0;

/* Camera locations and movement */
double currU = 0.0;
int currp0 = 1;
double cameraSpeedMin = 0.015;
double cameraSpeedMax = 0.055;
double dT = 0.01;

/* Camera orientation */
const vec v0{ 0.0, 0.0, -1.0 };
const vec v0alt{ 0.0316, 0.0316, -0.999 };
vec prevBinormal;

/* Custom variables */
const double uDiv = 0.001;
const double eyeOffset = 0.2;
const double distAcross = 0.1;
const double trackWidth = 0.02;
const double trackHeight = 0.02;
const double columnWidth = 0.04;
const double columnLength = 0.04;
const double crossSectionLen = 0.01;
const double columnSpread = 5.0;
const double crossbarLen = 0.04;
const double gravity = 9.8;

const bool _disableCameraMovement = false;

int loadSplines(char *argv) {
	char *cName = (char *)malloc(128 * sizeof(char));
	FILE *fileList;
	FILE *fileSpline;
	int iType, i = 0, j, iLength;

	/* load the track file */
	fileList = fopen(argv, "r");
	if (fileList == NULL) {
		printf("can't open file\n");
		exit(1);
	}

	/* stores the number of splines in a global variable */
	fscanf(fileList, "%d", &g_iNumOfSplines);
	printf("%d\n", g_iNumOfSplines);
	g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

	/* reads through the spline files */
	for (j = 0; j < g_iNumOfSplines; j++) {
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) {
			printf("can't open file\n");
			exit(1);
		}

		/* gets length for spline file */
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		/* allocate memory for all the points */
		g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
		g_Splines[j].numControlPoints = iLength;

		/* saves the data to the struct */
		while (fscanf(fileSpline, "%lf %lf %lf",
			&g_Splines[j].points[i].x,
			&g_Splines[j].points[i].y,
			&g_Splines[j].points[i].z) != EOF) {
			i++;
		}
	}

	free(cName);

	return 0;
}

/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename)
{
	if (filename == NULL)
		return;

	// Allocate a picture buffer // 
	cv::Mat3b bufferRGB = cv::Mat::zeros(480, 640, CV_8UC3); //rows, cols, 3-channel 8-bit.
	printf("File to save to: %s\n", filename);

	//use fast 4-byte alignment (default anyway) if possible
	glPixelStorei(GL_PACK_ALIGNMENT, (bufferRGB.step & 3) ? 1 : 4);
	//set length of one complete row in destination data (doesn't need to equal img.cols)
	glPixelStorei(GL_PACK_ROW_LENGTH, bufferRGB.step / bufferRGB.elemSize());
	glReadPixels(0, 0, bufferRGB.cols, bufferRGB.rows, GL_RGB, GL_UNSIGNED_BYTE, bufferRGB.data);
	//flip to account for GL 0,0 at lower left
	cv::flip(bufferRGB, bufferRGB, 0);
	//convert RGB to BGR
	cv::Mat3b bufferBGR(bufferRGB.rows, bufferRGB.cols, CV_8UC3);
	cv::Mat3b out[] = { bufferBGR };
	// rgb[0] -> bgr[2], rgba[1] -> bgr[1], rgb[2] -> bgr[0]
	int from_to[] = { 0,2, 1,1, 2,0 };
	mixChannels(&bufferRGB, 1, out, 1, from_to, 3);

	if (cv::imwrite(filename, bufferBGR)) {
		printf("File saved Successfully\n");
	}
	else {
		printf("Error in Saving\n");
	}
}

/* Function to get a pixel value. Use like PIC_PIXEL macro.
Note: OpenCV images are in channel order BGR.
This means that:
chan = 0 returns BLUE,
chan = 1 returns GREEN,
chan = 2 returns RED. */
unsigned char getPixelValue(cv::Mat3b& image, int x, int y, int chan)
{
	return image.at<cv::Vec3b>(y, x)[chan];
}

/* Function that does nothing but demonstrates looping through image coordinates.*/
void loopImage(cv::Mat3b& image)
{
	for (int r = 0; r < image.rows; r++) { // y-coordinate
		for (int c = 0; c < image.cols; c++) { // x-coordinate
			for (int channel = 0; channel < 3; channel++) {
				// DO SOMETHING... example usage
				// unsigned char blue = getPixelValue(image, c, r, 0);
				// unsigned char green = getPixelValue(image, c, r, 1); 
				// unsigned char red = getPixelValue(image, c, r, 2); 
			}
		}
	}
}

/* Read an image into memory.
Set argument displayOn to true to make sure images are loaded correctly.
One image loaded, set to false so it doesn't interfere with OpenGL window.*/
int readImage(char *filename, cv::Mat3b& image, bool displayOn)
{
	std::cout << "reading image: " << filename << std::endl;
	image = cv::imread(filename);
	if (!image.data) // Check for invalid input                    
	{
		std::cout << "Could not open or find the image." << std::endl;
		return 1;
	}

	if (displayOn)
	{
		cv::imshow("TestWindow", image);
		cv::waitKey(0); // Press any key to enter. 
	}
	return 0;
}

// called before main loop
void init()
{
	glClearColor(0.2, 0.2, 0.2, 0.0);   // set background color
	glEnable(GL_DEPTH_TEST);            // enable depth buffering
	glShadeModel(GL_SMOOTH);            // interpolate colors during rasterization
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(pov, (float)_windowWidth / _windowHeight, 0.01, 10000.0);
	glMatrixMode(GL_MODELVIEW);
}

void setUpCamera(point p, vec t, vec n, vec b, double dpdu) {
	vec e, c;
	e.x = p.x + eyeOffset * b.x;
	e.y = p.y + eyeOffset * b.y;
	e.z = p.z + eyeOffset * b.z;
	c.x = e.x + t.x;
	c.y = e.y + t.y;
	c.z = e.z + t.z;

	gluLookAt(e.x, e.y, e.z, c.x, c.y, c.z, b.x, b.y, b.z);
	
	if (_disableCameraMovement) {
		return;
	}
	
	// Move camera forward
	// Speed based on forward vector direction
	double dU = dT * sqrt(2 * gravity * (trackMaxHeight - p.z)) / dpdu;
	// Make sure the roller coaster stays moving
	dU = clamp(dU, cameraSpeedMin, cameraSpeedMax);
	currU += dU;

	// Restart the track if ended
	if (currU >= 1) {
		currU = 0;
		++currp0;
		if (currp0 >= g_Splines->numControlPoints - 2) {
			currp0 = 1;
		}
	}
}

void drawSkybox() {
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	
	glBindTexture(GL_TEXTURE_2D, textures[sky4ID]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0, 1.0); glVertex3f(-100.0, -100.0, -100.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-100.0, 100.0, -100.0);
	glTexCoord2f(0.0, 0.0); glVertex3f(-100.0, 100.0, 100.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(-100.0, -100.0, 100.0);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, textures[sky3ID]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0, 1.0); glVertex3f(-100.0, 100.0, -100.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(100.0, 100.0, -100.0);
	glTexCoord2f(0.0, 0.0); glVertex3f(100.0, 100.0, 100.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(-100.0, 100.0, 100.0);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, textures[sky2ID]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0, 1.0); glVertex3f(100.0, 100.0, -100.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(100.0, -100.0, -100.0);
	glTexCoord2f(0.0, 0.0); glVertex3f(100.0, -100.0, 100.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(100.0, 100.0, 100.0);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, textures[sky1ID]);
	glBegin(GL_QUADS);
	glTexCoord2f(1.0, 1.0); glVertex3f(100.0, -100.0, -100.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-100.0, -100.0, -100.0);
	glTexCoord2f(0.0, 0.0); glVertex3f(-100.0, -100.0, 100.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(100.0, -100.0, 100.0);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, textures[skyUpID]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-100.0, -100.0, 100.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(-100.0, 100.0, 100.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(100.0, 100.0, 100.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(100.0, -100.0, 100.0);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

void drawGround() {
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D, textures[groundID]);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex3f(-100.0, -100.0, -50.0);
	glTexCoord2f(0.0, 1.0); glVertex3f(-100.0, 100.0, -50.0);
	glTexCoord2f(1.0, 1.0); glVertex3f(100.0, 100.0, -50.0);
	glTexCoord2f(1.0, 0.0); glVertex3f(100.0, -100.0, -50.0);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void drawRails(vec t, vec n, vec b, bool leftRail) {
	glBegin(GL_QUADS);
	for (uint i = 0; i < _ridePoints.size() - crossSectionLen / uDiv; i += crossSectionLen / uDiv) {
		vec sideOffSet = leftRail ? (distAcross * n) : (-distAcross * n);
		point p0 = _ridePoints[i];
		point p1 = _ridePoints[i + crossSectionLen / uDiv];

		point v1 = p0 + trackWidth / 2 * n + trackHeight / 2 * b + sideOffSet;
		point v2 = p0 - trackWidth / 2 * n + trackHeight / 2 * b + sideOffSet;
		point v3 = p0 - trackWidth / 2 * n - trackHeight / 2 * b + sideOffSet;
		point v4 = p0 + trackWidth / 2 * n - trackHeight / 2 * b + sideOffSet;
		point v5 = p1 + trackWidth / 2 * n + trackHeight / 2 * b + sideOffSet;
		point v6 = p1 - trackWidth / 2 * n + trackHeight / 2 * b + sideOffSet;
		point v7 = p1 - trackWidth / 2 * n - trackHeight / 2 * b + sideOffSet;
		point v8 = p1 + trackWidth / 2 * n - trackHeight / 2 * b + sideOffSet;

		/**/
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v3.x, v3.y, v3.z);
		glVertex3f(v4.x, v4.y, v4.z);

		glVertex3f(v5.x, v5.y, v5.z);
		glVertex3f(v6.x, v6.y, v6.z);
		glVertex3f(v7.x, v7.y, v7.z);
		glVertex3f(v8.x, v8.y, v8.z);

		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v6.x, v6.y, v6.z);
		glVertex3f(v5.x, v5.y, v5.z);

		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v5.x, v5.y, v5.z);
		glVertex3f(v8.x, v8.y, v8.z);
		glVertex3f(v4.x, v4.y, v4.z);

		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v3.x, v3.y, v3.z);
		glVertex3f(v7.x, v7.y, v7.z);
		glVertex3f(v6.x, v6.y, v6.z);

		glVertex3f(v3.x, v3.y, v3.z);
		glVertex3f(v4.x, v4.y, v4.z);
		glVertex3f(v7.x, v7.y, v7.z);
		glVertex3f(v8.x, v8.y, v8.z);
	}
	glEnd();
}

void drawCrossbars(vec t, vec n) {
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, textures[woodID]);
	glBegin(GL_QUADS);
	for (uint i = 0; i < _ridePoints.size(); i += 400) {
		point p1 = _ridePoints[i] - distAcross * n;
		point p2 = _ridePoints[i] + distAcross * n;
		point p3 = _ridePoints[i] + distAcross * n + crossbarLen * t;
		point p4 = _ridePoints[i] - distAcross * n + crossbarLen * t;

		glTexCoord2f(0.0, 1.0); glVertex3f(p1.x, p1.y, p1.z);
		glTexCoord2f(1.0, 1.0); glVertex3f(p2.x, p2.y, p2.z);
		glTexCoord2f(1.0, 0.0); glVertex3f(p3.x, p3.y, p3.z);
		glTexCoord2f(0.0, 0.0); glVertex3f(p4.x, p4.y, p4.z);
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void drawColumns(vec n) {
	glBegin(GL_LINES);
	glColor3f(0.8, 0.0, 0.0);
	glLineWidth(100000);
	for (uint i = 0; i < _ridePoints.size() - columnSpread / uDiv; i += columnSpread / uDiv) {
		point v1 = _ridePoints[i] + distAcross * n;
		point v2{ v1.x , v1.y, -50.0 };
		point v3 = _ridePoints[i] - distAcross * n;
		point v4{ v3.x , v3.y, -50.0 };

		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glVertex3f(v3.x, v3.y, v3.z);
		glVertex3f(v4.x, v4.y, v4.z);
	}
	glEnd();
	glColor3f(1.0, 1.0, 1.0);
}

void drawTrack(vec t, vec n, vec b) {
	// Left cross sections
	drawRails(t, n, b, true);
	// Right cross sections
	drawRails(t, n, b, false);
	// Crossbars
	drawCrossbars(t, n);
	// Columns
	drawColumns(n);
}


// display a frame
void display()
{
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Calculate current position, normal, tangent, bitangent
	point p;
	vec t, n, b;
	p = CatmullRom(currU, g_Splines->points[currp0 - 1], g_Splines->points[currp0], g_Splines->points[currp0 + 1], g_Splines->points[currp0 + 2]);
	t = getForward(currU, g_Splines->points[currp0 - 1], g_Splines->points[currp0], g_Splines->points[currp0 + 1], g_Splines->points[currp0 + 2]);
	double dpdu = size(t);
	t = normalize(t);

	if (currU == 0.0 && currp0 == 1) {
		n = cross(t, v0);
		if (IsZero(n)) {
			n = cross(t, v0alt);
		}
		n = normalize(n);
		b = prevBinormal = normalize(cross(t, n));
	}
	else {
		n = normalize(cross(prevBinormal, t));
		b = prevBinormal = normalize(cross(t, n));
	}

	// Input translation
	glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
	glRotatef(g_vLandRotate[0], 1, 0, 0);
	glRotatef(g_vLandRotate[1], 0, 1, 0);
	glRotatef(g_vLandRotate[2], 0, 0, 1);
	glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);


	// Set up camera
	setUpCamera(p, t, n, b, dpdu);

	// Draw ground
	drawGround();
	// Draw skybox
	drawSkybox();

	// Draw track
	drawTrack(t, n, b);

	glPopMatrix();
	glutSwapBuffers(); // double buffer flush
}

void doIdle()
{
	/* do some stuff... */

	/* make the screen update */
	glutPostRedisplay();
}

void menufunc(int value)
{
	switch (value)
	{
	case 0:
		exit(0);
		break;
	}
}

/* converts mouse drags into information about
rotation/translation/scaling */
void mousedrag(int x, int y)
{
	int vMouseDelta[2] = { x - g_vMousePos[0], y - g_vMousePos[1] };

	switch (g_ControlState)
	{
	case TRANSLATE:
		if (g_iLeftMouseButton)
		{
			g_vLandTranslate[0] += vMouseDelta[0] * 0.01;
			g_vLandTranslate[1] -= vMouseDelta[1] * 0.01;
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandTranslate[2] += vMouseDelta[1] * 0.01;
		}
		break;
	case ROTATE:
		if (g_iLeftMouseButton)
		{
			g_vLandRotate[0] += vMouseDelta[1];
			g_vLandRotate[1] += vMouseDelta[0];
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandRotate[2] += vMouseDelta[1];
		}
		break;
	case SCALE:
		if (g_iLeftMouseButton)
		{
			g_vLandScale[0] *= 1.0 + vMouseDelta[0] * 0.01;
			g_vLandScale[1] *= 1.0 - vMouseDelta[1] * 0.01;
		}
		if (g_iMiddleMouseButton)
		{
			g_vLandScale[2] *= 1.0 - vMouseDelta[1] * 0.01;
		}
		break;
	}
	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		g_iLeftMouseButton = (state == GLUT_DOWN);
		break;
	case GLUT_MIDDLE_BUTTON:
		g_iMiddleMouseButton = (state == GLUT_DOWN);
		break;
	case GLUT_RIGHT_BUTTON:
		g_iRightMouseButton = (state == GLUT_DOWN);
		break;
	}

	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		g_ControlState = TRANSLATE;
		break;
	case GLUT_ACTIVE_SHIFT:
		g_ControlState = SCALE;
		break;
	default:
		g_ControlState = ROTATE;
		break;
	}

	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

// Keyboard input
//void keyboardpress(unsigned char key, int x, int y) {
//	switch (key)
//	{
//	case '1':
//		_displayMode = disp_TRIANGLES;
//		break;
//	case '2':
//		_displayMode = disp_WIREFRAME;
//		break;
//	case '3':
//		_displayMode = disp_POINTS;
//		break;
//	case '4':
//		_displayMode = disp_TRIANGLES_AND_WIREFRAME;
//		break;
//	default:
//		break;
//	}
//}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
	{
		printf("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	// Load data into g_Splines
	loadSplines(argv[1]);

	// Calculate points
	for (int i = 1; i < g_Splines->numControlPoints - 2; ++i) {
		for (double u = 0; u < 1; u += uDiv) {
			_ridePoints.push_back(CatmullRom(u, g_Splines->points[i - 1], g_Splines->points[i], g_Splines->points[i + 1], g_Splines->points[i + 2]));
		}
	}

	for (int i = 0; i < _ridePoints.size(); ++i) {
		if (_ridePoints[i].z > trackMaxHeight) {
			trackMaxHeight = _ridePoints[i].z;
		}
	}

	// initialize GLUT
	glutInit(&argc, argv);

	// request double buffer
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);

	// set window size
	glutInitWindowSize(500, 500);

	// set window position
	glutInitWindowPosition(0, 0);

	// creates a window
	glutCreateWindow(argv[0]);

	/* allow the user to quit using the right mouse button menu */
	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// GLUT callbacks
	glutDisplayFunc(display);
	glutIdleFunc(doIdle);
	glutMotionFunc(mousedrag);
	glutPassiveMotionFunc(mouseidle);
	glutMouseFunc(mousebutton);
	//glutKeyboardFunc(keyboardpress);

	// initialize states
	init();

	// Load textures
	readImage("groundtexture.jpg", groundTexture, false);
	readImage("skyup.jpg", skyUpTexture, false);
	readImage("woodtexture.jpg", woodTexture, false);
	readImage("sky1.jpg", sky1Texture, false);
	readImage("sky2.jpg", sky2Texture, false);
	readImage("sky3.jpg", sky3Texture, false);
	readImage("sky4.jpg", sky4Texture, false);

	glGenTextures(sizeof(textures) / sizeof(GLuint), textures);

	glBindTexture(GL_TEXTURE_2D, textures[groundID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, groundTexture.rows, groundTexture.cols, 0, GL_RGB, GL_UNSIGNED_BYTE, groundTexture.data);

	glBindTexture(GL_TEXTURE_2D, textures[woodID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, woodTexture.rows, woodTexture.cols, 0, GL_RGB, GL_UNSIGNED_BYTE, woodTexture.data);

	glBindTexture(GL_TEXTURE_2D, textures[skyUpID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, skyUpTexture.rows, skyUpTexture.cols, 0, GL_RGB, GL_UNSIGNED_BYTE, skyUpTexture.data);

	glBindTexture(GL_TEXTURE_2D, textures[sky1ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sky1Texture.rows, sky1Texture.cols, 0, GL_RGB, GL_UNSIGNED_BYTE, sky1Texture.data);

	glBindTexture(GL_TEXTURE_2D, textures[sky2ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sky2Texture.rows, sky2Texture.cols, 0, GL_RGB, GL_UNSIGNED_BYTE, sky2Texture.data);

	glBindTexture(GL_TEXTURE_2D, textures[sky3ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sky3Texture.rows, sky3Texture.cols, 0, GL_RGB, GL_UNSIGNED_BYTE, sky3Texture.data);

	glBindTexture(GL_TEXTURE_2D, textures[sky4ID]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sky4Texture.rows, sky4Texture.cols, 0, GL_RGB, GL_UNSIGNED_BYTE, sky4Texture.data);

	// start GLUT program
	glutMainLoop();

	return 0;
}
