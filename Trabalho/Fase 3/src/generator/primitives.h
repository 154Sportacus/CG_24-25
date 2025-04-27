#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <vector>
#include <array>
#include <string>

// Each vertex is represented by an array of 3 floats.
typedef std::array<float, 3> Vertex;

// Generates the vertices for a plane centered at the origin.
std::vector<Vertex> generatePlane(float dimension, int divisions);

// Generates the vertices for a sphere centered at the origin.
std::vector<Vertex> generateSphere(float radius, int slices, int stacks);

// Generates the vertices for a cube centered at the origin.
std::vector<Vertex> generateCube(float dimension, int divisions);

// Generates the vertices for a cone with its base on the XZ plane.
std::vector<Vertex> generateCone(float bottomRadius, float height, int slices, int stacks);

// Generates the vertices for a ring in the XZ plane, centered at the origin.
std::vector<Vertex> generateRing(float outerRadius, float innerRadius, int slices);


// Writes the vertices to a .3d file in the "models/generated/" directory.
void writeVertices(const std::vector<Vertex> &verts, const std::string &filename);

#endif // PRIMITIVES_H
