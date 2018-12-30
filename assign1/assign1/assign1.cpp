// assign1.cpp : Defines the entry point for the console application.
//

/*
  CSCI 480 Computer Graphics
  Assignment 1: Height Fields
  C++ starter code
*/


#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "CImg-2.3.5\CImg.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

using namespace cimg_library;
using namespace std;

int g_iMenuId;

int _windowWidth = 640;
int _windowHeight = 480;

int g_vMousePos[2] = { 0, 0 };
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = { 0.0, 0.0, 0.0 };
float g_vLandTranslate[3] = { 0.0, 0.0, 0.0 };
float g_vLandScale[3] = { 1.0, 1.0, 1.0 };

enum DISPLAYMODE {
	disp_TRIANGLES,
	disp_WIREFRAME,
	disp_POINTS,
	disp_TRIANGLES_AND_WIREFRAME
};

DISPLAYMODE _displayMode = disp_TRIANGLES;

CImg<unsigned char> *g_pHeightData;

int imgWidth = 0;
int imgHeight = 0;
int imgDepth = 0;

/* This line is required for CImg to be able to read jpg/png format files. */
/* Please install ImageMagick and replace the path below to the correct path to convert.exe on your computer */
void initializeImageMagick()
{
	cimg::imagemagick_path("convert.exe", true);
}


/* Write a screenshot to the specified filename */
void saveScreenshot(char *filename)
{
	int i, j;

	if (filename == NULL)
		return;

	/* Allocate a picture buffer */
	CImg<unsigned char> in(640, 480, 1, 3, 0);

	printf("File to save to: %s\n", filename);

	for (i = 479; i >= 0; i--) {
		glReadPixels(0, 479 - i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
			in.data());
	}

	if (in.save_jpeg(filename))
		printf("File saved Successfully\n");
	else
		printf("Error in Saving\n");

}

void myinit()
{
	/* setup gl view here */
	glClearColor(0.2, 0.2, 0.2, 0.0);   // set background color
	glEnable(GL_DEPTH_TEST);            // enable depth buffering
	glShadeModel(GL_SMOOTH);            // interpolate colors during rasterization
	gluPerspective(90, (float)_windowWidth/_windowHeight, 0.01, 10000.0);
}

void display()
{
	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Input translation
	glTranslatef(g_vLandTranslate[0], g_vLandTranslate[1], g_vLandTranslate[2]);
	glRotatef(g_vLandRotate[0], 1, 0, 0);
	glRotatef(g_vLandRotate[1], 0, 0, 1);
	glRotatef(g_vLandRotate[2], 0, 1, 0);
	glScalef(g_vLandScale[0], g_vLandScale[1], g_vLandScale[2]);

	// Scene object translation
	glScalef(0.005f, 0.005f, 0.0005f);
	glTranslatef(-imgWidth / 2.f, -imgHeight / 2.f, 0);
	
	// Draw object
	switch (_displayMode) {
	case disp_TRIANGLES:
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < imgHeight - 1; ++i) {
			for (int j = 0; j < imgWidth - 1; ++j) {
				float v1Height = (float)(*g_pHeightData)(i, j);
				float v2Height = (float)(*g_pHeightData)(i + 1, j);
				float v3Height = (float)(*g_pHeightData)(i, j + 1);
				float v4Height = (float)(*g_pHeightData)(i + 1, j + 1);
				glColor3f(v1Height / 255.f, v1Height / 255.f, 1.0);
				glVertex3f(i, j, v1Height);
				glColor3f(v2Height / 255.f, v2Height / 255.f, 1.0);
				glVertex3f(i + 1, j, v2Height);
				glColor3f(v3Height / 255.f, v3Height / 255.f, 1.0);
				glVertex3f(i, j + 1, v3Height);

				glColor3f(v2Height / 255.f, v2Height / 255.f, 1.0);
				glVertex3f(i + 1, j, v2Height);
				glColor3f(v4Height / 255.f, v4Height / 255.f, 1.0);
				glVertex3f(i + 1, j + 1, v4Height);
				glColor3f(v3Height / 255.f, v3Height / 255.f, 1.0);
				glVertex3f(i, j + 1, v3Height);
			}
		}
		glEnd();
		break;

	case disp_WIREFRAME:
		glBegin(GL_LINES);
		glColor3f(0.0, 0.75, 0.0);
		for (int i = 0; i < imgHeight - 1; ++i) {
			for (int j = 0; j < imgWidth - 1; ++j) {
				float v1Height = (float)(*g_pHeightData)(i, j);
				float v2Height = (float)(*g_pHeightData)(i + 1, j);
				float v3Height = (float)(*g_pHeightData)(i, j + 1);
				float v4Height = (float)(*g_pHeightData)(i + 1, j + 1);
				glVertex3f(i, j, v1Height);
				glVertex3f(i + 1, j, v2Height);

				glVertex3f(i + 1, j, v2Height);
				glVertex3f(i, j + 1, v3Height);

				glVertex3f(i, j + 1, v3Height);
				glVertex3f(i, j, v1Height);

				glVertex3f(i + 1, j + 1, v4Height);
				glVertex3f(i, j + 1, v3Height);

				glVertex3f(i + 1, j + 1, v4Height);
				glVertex3f(i + 1, j, v2Height);
			}
		}
		glEnd();
		break;

	case disp_POINTS:
		glBegin(GL_POINTS);
		for (int i = 0; i < imgHeight - 1; ++i) {
			for (int j = 0; j < imgWidth - 1; ++j) {
				float vertexHeight = (float)(*g_pHeightData)(i, j);
				glColor3f(vertexHeight / 255.f, vertexHeight / 255.f, 1.0);
				glVertex3f(i, j, vertexHeight);
			}
		}
		glEnd();
		break;

	case disp_TRIANGLES_AND_WIREFRAME:
		glBegin(GL_TRIANGLES);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 1.0);
		for (int i = 0; i < imgHeight - 1; ++i) {
			for (int j = 0; j < imgWidth - 1; ++j) {
				float v1Height = (float)(*g_pHeightData)(i, j);
				float v2Height = (float)(*g_pHeightData)(i + 1, j);
				float v3Height = (float)(*g_pHeightData)(i, j + 1);
				float v4Height = (float)(*g_pHeightData)(i + 1, j + 1);
				glColor3f(v1Height / 255.f, v1Height / 255.f, 1.0);
				glVertex3f(i, j, v1Height);
				glColor3f(v2Height / 255.f, v2Height / 255.f, 1.0);
				glVertex3f(i + 1, j, v2Height);
				glColor3f(v3Height / 255.f, v3Height / 255.f, 1.0);
				glVertex3f(i, j + 1, v3Height);

				glColor3f(v2Height / 255.f, v2Height / 255.f, 1.0);
				glVertex3f(i + 1, j, v2Height);
				glColor3f(v4Height / 255.f, v4Height / 255.f, 1.0);
				glVertex3f(i + 1, j + 1, v4Height);
				glColor3f(v3Height / 255.f, v3Height / 255.f, 1.0);
				glVertex3f(i, j + 1, v3Height);
			}
		}
		glDisable(GL_POLYGON_OFFSET_FILL);
		glEnd();
		glBegin(GL_LINES);
		glColor3f(0.0, 0.75, 0.0);
		for (int i = 0; i < imgHeight - 1; ++i) {
			for (int j = 0; j < imgWidth - 1; ++j) {
				float v1Height = (float)(*g_pHeightData)(i, j);
				float v2Height = (float)(*g_pHeightData)(i + 1, j);
				float v3Height = (float)(*g_pHeightData)(i, j + 1);
				float v4Height = (float)(*g_pHeightData)(i + 1, j + 1);
				glVertex3f(i, j, v1Height);
				glVertex3f(i + 1, j, v2Height);

				glVertex3f(i + 1, j, v2Height);
				glVertex3f(i, j + 1, v3Height);

				glVertex3f(i, j + 1, v3Height);
				glVertex3f(i, j, v1Height);

				glVertex3f(i + 1, j + 1, v4Height);
				glVertex3f(i, j + 1, v3Height);

				glVertex3f(i + 1, j + 1, v4Height);
				glVertex3f(i + 1, j, v2Height);
			}
		}
		glEnd();
		break;
	}

	glutSwapBuffers(); // double buffer flush
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

int frameCounter = 0;
int screenshotCounter = 0;
void doIdle()
{
	/* do some stuff... */

	/* make the screen update */
	glutPostRedisplay();
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

void keyboardpress(unsigned char key, int x, int y) {
	switch (key)
	{
	case '1':
		_displayMode = disp_TRIANGLES;
		break;
	case '2':
		_displayMode = disp_WIREFRAME;
		break;
	case '3':
		_displayMode = disp_POINTS;
		break;
	case '4':
		_displayMode = disp_TRIANGLES_AND_WIREFRAME;
		break;
	default:
		break;
	}
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("usage: %s heightfield.jpg\n", argv[0]);
		exit(1);
	}


	// Read image
	initializeImageMagick();

	g_pHeightData = new CImg<unsigned char>((char*)argv[1]);
	if (!g_pHeightData)
	{
		printf("error reading %s.\n", argv[1]);
		exit(1);
	}

	imgWidth = g_pHeightData->width();
	imgHeight = g_pHeightData->height();
	imgDepth = g_pHeightData->depth();
	// Done reading image

	glutInit(&argc, (char**)argv);

	/*
		create a window here..should be double buffered and use depth testing

		the code past here will segfault if you don't have a window set up....
		replace the exit once you add those calls.
	*/
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(_windowWidth, _windowHeight);
	glutInitWindowPosition(100, 100);
	glutCreateWindow(argv[0]);

	//exit(0);

	/* tells glut to use a particular display function to redraw */
	glutDisplayFunc(display);

	/* allow the user to quit using the right mouse button menu */
	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit", 0);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/* replace with any animate code */
	glutIdleFunc(doIdle);

	/* callback for mouse drags */
	glutMotionFunc(mousedrag);
	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);
	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);
	/* callback for keyboard button presses */
	glutKeyboardFunc(keyboardpress);

	/* do initialization */
	myinit();

	glutMainLoop();
	return 0;
}