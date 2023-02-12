#include <GL/glut.h>

#include "window.h"

Window *windowHandle=NULL; // HACK but we only use a single window so it works

void windowDisplayWrapper(void);
void windowReshapeWrapper(int w, int h);

Window::Window() {
	windowHandle=this; // TODO: may want to check to make sure this was previously NULL
	width=0;
	height=0;
	id=0;

	glutInitDisplayMode(GLUT_DOUBLE|GLUT_DEPTH);
	glutInitWindowSize(640, 480);
	glutInitWindowPosition(100, 100);
	id=glutCreateWindow("Game");
	glutFullScreen();

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glEnable(GL_DEPTH_TEST);

	glutDisplayFunc(windowDisplayWrapper);
	glutReshapeFunc(windowReshapeWrapper);
}

Window::~Window() {
	glutDestroyWindow(id);
}

void Window::displayHandler(void) {
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, -2, 2);
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();

	glBegin(GL_POLYGON);
	glVertex3f(0.5, 0.0, 0.5);
	glVertex3f(0.5, 0.0, 0.0);
	glVertex3f(0.0, 0.5, 0.0);
	glVertex3f(0.0, 0.0, 0.5);
	glEnd();

	glutSwapBuffers();
}

void Window::reshapeHandler(int w, int h) {
	width=w;
	height=h;

	glViewport(0, 0, width, height);
}

void windowDisplayWrapper(void) {
	if (windowHandle!=NULL)
		windowHandle->displayHandler();
}

void windowReshapeWrapper(int w, int h) {
	if (windowHandle!=NULL)
		windowHandle->reshapeHandler(w, h);
}
