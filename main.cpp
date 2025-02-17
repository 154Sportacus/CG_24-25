#include <stdlib.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif




#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <array>
#include <functional>
#include <fstream> 


GLenum glMode = GL_TRIANGLES;

GLenum polygonFace = GL_FRONT;
GLenum polygonModeSetting = GL_FILL;

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
	case 'o': // Toggle between filled and wireframe mode
		if (polygonModeSetting == GL_LINE)
			polygonModeSetting = GL_FILL;
		else
			polygonModeSetting = GL_LINE;
		glPolygonMode(polygonFace, polygonModeSetting);
		break;

		// Exit the Window
	case 27: exit(0); // ESC key exits
	default:
		break;
	}
	glutPostRedisplay(); // Redraw scene
}

void drawPlane(float dimension, int divisions) {
	float half = dimension * 0.5f;       // Half-dimension: plane extends from -half to +half.
	float step = dimension / divisions;  // Size of each subdivided cell.

	for (int i = 0; i < divisions; i++) {
		for (int j = 0; j < divisions; j++) {
			// Compute the four corners of the current cell.
			float x0 = -half + i * step;
			float x1 = -half + (i + 1) * step;
			float z0 = -half + j * step;
			float z1 = -half + (j + 1) * step;

			// Draw two triangles to form the cell.
			glBegin(GL_TRIANGLES);
			glVertex3f(x0, 0.0f, z0);
			glVertex3f(x1, 0.0f, z0);
			glVertex3f(x1, 0.0f, z1);
			glEnd();

			glBegin(GL_TRIANGLES);
			glVertex3f(x0, 0.0f, z0);
			glVertex3f(x1, 0.0f, z1);
			glVertex3f(x0, 0.0f, z1);
			glEnd();
		}
	}
}


void drawSphere(float radius, int slices, int stacks) {
	// Open/create "sphere.3d" for writing.
	std::ofstream file("C:/Users/gugac/Universidade/24-25/2nd Semestre/CG/3d Files/sphere.3d");

	if (!file.is_open()) {
		fprintf(stderr, "Error: Could not open sphere.3d for writing.\n");
		return;
	}

	float step_stacks = M_PI / float(stacks);
	float step_slices = 2 * M_PI / float(slices);

	// Loop over stacks and slices:
	for (int i = 0; i < stacks; i++) {
		float lat1 = i * step_stacks - M_PI / 2;
		float lat2 = (i + 1) * step_stacks - M_PI / 2;
		for (int j = 0; j < slices; j++) {
			float angle = j * step_slices;
			float angle2 = (j + 1) * step_slices;

			// Triangle 1
			glBegin(GL_TRIANGLES);
			// 1st vertex
			float x1 = radius * cosf(lat1) * cosf(angle);
			float y1 = radius * sinf(lat1);
			float z1 = radius * cosf(lat1) * sinf(angle);
			glVertex3f(x1, y1, z1);
			file << x1 << " " << y1 << " " << z1 << "\n";

			// 2nd vertex
			float x2 = radius * cosf(lat2) * cosf(angle);
			float y2 = radius * sinf(lat2);
			float z2 = radius * cosf(lat2) * sinf(angle);
			glVertex3f(x2, y2, z2);
			file << x2 << " " << y2 << " " << z2 << "\n";

			// 3rd vertex
			float x3 = radius * cosf(lat2) * cosf(angle2);
			float y3 = radius * sinf(lat2);
			float z3 = radius * cosf(lat2) * sinf(angle2);
			glVertex3f(x3, y3, z3);
			file << x3 << " " << y3 << " " << z3 << "\n";
			glEnd();

			// Triangle 2
			glBegin(GL_TRIANGLES);
			// 1st vertex (== 1st vertex of triangle 1=
			glVertex3f(x1, y1, z1);
			file << x1 << " " << y1 << " " << z1 << "\n";

			// 2nd vertex (== 3rd vertex of triangle 1)
			glVertex3f(x3, y3, z3);
			file << x3 << " " << y3 << " " << z3 << "\n";

			// 3rd vertex
			float x6 = radius * cosf(lat1) * cosf(angle2);
			float y6 = radius * sinf(lat1);
			float z6 = radius * cosf(lat1) * sinf(angle2);
			glVertex3f(x6, y6, z6);
			file << x6 << " " << y6 << " " << z6 << "\n";
			glEnd();
		}
	}

	// Close the file after writing all vertex data.
	file.close();
}


void drawCylinder(float radius, float height, int slices) {
	float step = 2 * M_PI / float(slices);

	for (float i = 0; i < slices; i++) {

		float angle = i * step;
		float angle2 = (i + 1) * step;

		glBegin(GL_TRIANGLES);
		// top base
		glVertex3f(radius * cosf(angle), height, radius * sinf(angle));
		glVertex3f(0.0f, height, 0.0f);
		glVertex3f(radius * cosf(angle2), height, radius * sinf(angle2));

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

typedef std::array<float, 3> Vec3;

void drawCube(float dimension, int divisions) {
	float half = dimension * 0.5f;       // Half the dimension.
	float step = dimension / divisions;  // Size of each subdivision.

	// lambdas for each face.
	auto frontTransform = [=](float x, float y, float z) -> Vec3 {
		return { x, y, z };  // Identity.
		};
	auto backTransform = [=](float x, float y, float z) -> Vec3 {
		// Rotate 180° about the y-axis: (x, y, z) -> (-x, y, -z)
		return { -x, y, -z };
		};
	auto rightTransform = [=](float x, float y, float z) -> Vec3 {
		// Rotate 90° about the y-axis:
		//   x' = cos(90)*x + sin(90)*z = 0*x + 1*z = z,
		//   y' = y,
		//   z' = -sin(90)*x + cos(90)*z = -1*x + 0*z = -x.
		return { z, y, -x };
		};
	auto leftTransform = [=](float x, float y, float z) -> Vec3 {
		// Rotate -90° about the y-axis:
		//   x' = cos(-90)*x + sin(-90)*z = 0*x - 1*z = -z,
		//   y' = y,
		//   z' = -sin(-90)*x + cos(-90)*z = 1*x + 0*z = x.
		return { -z, y, x };
		};
	auto topTransform = [=](float x, float y, float z) -> Vec3 {
		// Rotate -90° about the x-axis:
		//   x' = x,
		//   y' = cos(-90)*y - sin(-90)*z = 0*y - (-1)*z = z,
		//   z' = sin(-90)*y + cos(-90)*z = -1*y + 0*z = -y.
		return { x, z, -y };
		};
	auto bottomTransform = [=](float x, float y, float z) -> Vec3 {
		// Rotate 90° about the x-axis:
		//   x' = x,
		//   y' = cos(90)*y - sin(90)*z = 0*y - 1*z = -z,
		//   z' = sin(90)*y + cos(90)*z = 1*y + 0*z = y.
		return { x, -z, y };
		};

	// Pack the face transformations in a vector.
	struct Face {
		std::function<Vec3(float, float, float)> transform;
	};
	std::vector<Face> faces = {
		{ frontTransform },
		{ backTransform },
		{ rightTransform },
		{ leftTransform },
		{ topTransform },
		{ bottomTransform }
	};

	// Iterate over the subdivisions for each face.
	for (const auto& face : faces) {
		for (int i = 0; i < divisions; i++) {
			for (int j = 0; j < divisions; j++) {
				// Coords for the front face.
				float x0 = -half + i * step;
				float x1 = -half + (i + 1) * step;
				float y0 = -half + j * step;
				float y1 = -half + (j + 1) * step;
				float z = half; // Front face constant z.

				// Transform the four corners using the face's transformation.
				Vec3 v0 = face.transform(x0, y0, z);
				Vec3 v1 = face.transform(x1, y0, z);
				Vec3 v2 = face.transform(x1, y1, z);
				Vec3 v3 = face.transform(x0, y1, z);

				// Draw two triangles.
				glBegin(GL_TRIANGLES);
				glVertex3f(v0[0], v0[1], v0[2]);
				glVertex3f(v1[0], v1[1], v1[2]);
				glVertex3f(v2[0], v2[1], v2[2]);
				glEnd();

				glBegin(GL_TRIANGLES);
				glVertex3f(v0[0], v0[1], v0[2]);
				glVertex3f(v2[0], v2[1], v2[2]);
				glVertex3f(v3[0], v3[1], v3[2]);
				glEnd();
			}
		}
	}
}

void drawCone(float bottomRadius, float height, int slices, int stacks) {

	for (int j = 0; j < slices; j++) {
		float angle0 = 2.0f * M_PI * j / slices;
		float angle1 = 2.0f * M_PI * (j + 1) / slices;
		glBegin(GL_TRIANGLES);
		
		glVertex3f(bottomRadius * cosf(angle1), 0.0f, bottomRadius * sinf(angle1));
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(bottomRadius * cosf(angle0), 0.0f, bottomRadius * sinf(angle0));
		glEnd();
	}

	// Lateral surface
	for (int i = 0; i < stacks; i++) {
		float f0 = (float)i / stacks, f1 = (float)(i + 1) / stacks;
		float y0 = f0 * height, y1 = f1 * height;
		float r0 = bottomRadius * (1.0f - f0), r1 = bottomRadius * (1.0f - f1);
		for (int j = 0; j < slices; j++) {
			float angle0 = 2.0f * M_PI * j / slices;
			float angle1 = 2.0f * M_PI * (j + 1) / slices;
			float x0 = r0 * cosf(angle0), z0 = r0 * sinf(angle0);
			float x1 = r0 * cosf(angle1), z1 = r0 * sinf(angle1);
			float x2 = r1 * cosf(angle1), z2 = r1 * sinf(angle1);
			float x3 = r1 * cosf(angle0), z3 = r1 * sinf(angle0);
			if (i < stacks - 1) {
				glBegin(GL_TRIANGLES);
				glVertex3f(x0, y0, z0);
				glVertex3f(x2, y1, z2);
				glVertex3f(x1, y0, z1);
				glEnd();
				glBegin(GL_TRIANGLES);
				glVertex3f(x0, y0, z0);
				glVertex3f(x3, y1, z3);
				glVertex3f(x2, y1, z2);
				glEnd();
			}
			else {
				glBegin(GL_TRIANGLES);
				glVertex3f(x0, y0, z0);
				glVertex3f(0.0f, height, 0.0f);
				glVertex3f(x1, y0, z1);
				glEnd();
			}
		}
	}
}




void renderScene(void) {

	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set the camera
	glLoadIdentity();
	gluLookAt(::camera.eye[0], ::camera.eye[1], ::camera.eye[2],
		::camera.center[0], ::camera.center[1], ::camera.center[2],
		::camera.up[0], ::camera.up[1], ::camera.up[2]);

	//drawCylinder(1,2,10);

	//drawSphere(1,10,10);
	
	drawCube(3, 10);

	drawCone(1.0f, 2.0f, 4, 3);

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
	//glCullFace(GL_BACK);

	
// enter GLUT's main cycle
	glutMainLoop();
	
	return 1;
}

