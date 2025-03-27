#include <stdlib.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif




#define _USE_MATH_DEFINES
#include <math.h>
struct Camera {
	float eye[3];    // Camera position
	float center[3]; // Look-at point (always 0,0,0)
	float up[3];     // Up vector
	float yaw;       // Rotation around Y-axis (horizontal)
	float pitch;     // Rotation around X-axis (vertical)
	float distance;  // Distance from (0,0,0) (for zoom)

	// Constructor
	Camera() {
		yaw = 0.0f;          // Start facing along -Z axis
		pitch = 0.0f;        // No vertical tilt initially
		distance = 5.0f;     // Default distance from (0,0,0)

		center[0] = 0.0f; center[1] = 0.0f; center[2] = 0.0f;
		up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f;

		updatePosition(); // Initialize `eye[]` based on yaw and distance
	}

	// Update eye position based on yaw, pitch, and distance
	void updatePosition() {
		eye[0] = distance * cos(pitch) * cos(yaw);  // X-axis (horizontal)
		eye[1] = distance * sin(pitch);            // Y-axis (vertical)
		eye[2] = distance * cos(pitch) * sin(yaw); // Z-axis (depth)

		// Always look at (0,0,0)
		center[0] = 0.0f;
		center[1] = 0.0f;
		center[2] = 0.0f;

		// Adjust up vector based on pitch
		up[0] = -sin(pitch) * cos(yaw);
		up[1] = cos(pitch);
		up[2] = -sin(pitch) * sin(yaw);
	}

	// Rotate horizontally (yaw)
	void rotateYaw(float angle) {
		yaw += angle;
		updatePosition();
	}

	// Rotate vertically (pitch)
	void rotatePitch(float angle) {
		pitch += angle;

		// Prevent flipping upside down
		if (pitch > M_PI_2 - 0.1f) pitch = M_PI_2 - 0.1f;
		if (pitch < -M_PI_2 + 0.1f) pitch = -M_PI_2 + 0.1f;

		updatePosition();
	}

	// Zoom (change distance to (0,0,0))
	void zoom(float amount) {
		distance += amount;
		if (distance < 2.0f) distance = 2.0f;  // Prevent extreme zoom-in
		if (distance > 15.0f) distance = 15.0f; // Prevent extreme zoom-out
		updatePosition();
	}
};

// Global camera instance
Camera camera;
void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window with zero width).
	if(h == 0)
		h = 1;

	// compute window's aspect ratio 
	float ratio = w * 1.0 / h;

	// Set the projection matrix as current
	glMatrixMode(GL_PROJECTION);
	// Load Identity Matrix
	glLoadIdentity();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set perspective
	gluPerspective(45.0f ,ratio, 1.0f ,1000.0f);

	// return to the model view matrix mode
	glMatrixMode(GL_MODELVIEW);
}


void drawCylinder(float radius, float height, int slices) {
	float step = 2 * M_PI / float(slices);

	for (float i = 0; i<slices; i++) {
		
		float angle = i * step;
		float angle2 = (i + 1) * step;

		glBegin(GL_TRIANGLES);
		// top base
		glVertex3f(radius*cosf(angle), height, radius*sinf(angle));
		glVertex3f(0.0f, height, 0.0f);
		glVertex3f(radius*cosf(angle2), height, radius*sinf(angle2));

		// bottom base
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(radius * cosf(angle), 0.0f, radius * sinf(angle));
		glVertex3f(radius * cosf(angle2), 0.0f, radius * sinf(angle2));

		// sides
		glVertex3f(radius * cosf(angle), 0.0f, radius * sinf(angle));  // Bottom Left
		glVertex3f(radius * cosf(angle), height, radius * sinf(angle)); // Top Left
		glVertex3f(radius * cosf(angle2), height, radius * sinf(angle2)); // Top Right

		glVertex3f(radius * cosf(angle), 0.0f, radius * sinf(angle));  // Bottom Left
		glVertex3f(radius * cosf(angle2), height, radius * sinf(angle2)); // Top Right
		glVertex3f(radius * cosf(angle2), 0.0f, radius * sinf(angle2)); // Bottom Right

		glEnd();

	}

}


static char lowerMyKey(unsigned char key) {
	return (key >= 'A' && key <= 'Z') ? key + 0x20 : key;
}

void keyboard(unsigned char key, int x, int y) {
	float rotateStep = 0.05f; // Rotation speed
	float zoomStep = 0.3f;    // Zoom speed

	float rotateStepPyramid = rotateStep * 20; // Just to make it a bit sharper ;)
	// Keys For Camera Movement and Rotation
	switch (lowerMyKey(key)) {
	case 'a': camera.rotateYaw(-rotateStep); break;  // Rotate left
	case 'd': camera.rotateYaw(rotateStep); break;   // Rotate right
	case 'w': camera.rotatePitch(rotateStep); break; // Look up
	case 's': camera.rotatePitch(-rotateStep); break;// Look down
	case 'x': camera.zoom(-zoomStep); break;         // Zoom in
	case 'z': camera.zoom(zoomStep); break;          // Zoom out

		// Exit the Window
	case 27: exit(0); // ESC key exits
	}

	glutPostRedisplay(); // Redraw scene
}

void renderScene(void) {

	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set the camera
	glLoadIdentity();
	gluLookAt(::camera.eye[0], ::camera.eye[1], ::camera.eye[2],
		::camera.center[0], ::camera.center[1], ::camera.center[2],
		::camera.up[0], ::camera.up[1], ::camera.up[2]);

	drawCylinder(1,2,10);

	// End of frame
	glutSwapBuffers();
}


void processKeys(unsigned char c, int xx, int yy) {

// put code to process regular keys in here

}


void processSpecialKeys(int key, int xx, int yy) {

// put code to process special keys in here

}


int main(int argc, char **argv) {

// init GLUT and the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(800,800);
	glutCreateWindow("CG@DI-UM");
		
// Required callback registry 
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	
// Callback registration for keyboard processing
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(processSpecialKeys);

//  OpenGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	
// enter GLUT's main cycle
	glutMainLoop();
	
	return 1;
}
