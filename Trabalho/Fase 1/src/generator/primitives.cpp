#include "primitives.h"
#include <fstream>
#include <iostream>
#include <cmath>

//-------------------------------------------------------------------------
// Plane
//-------------------------------------------------------------------------
std::vector<Vertex> generatePlane(float dimension, int divisions) {
    std::vector<Vertex> verts;
    float half = dimension * 0.5f;
    float step = dimension / divisions;
    for (int i = 0; i < divisions; i++) {
        for (int j = 0; j < divisions; j++) {
            float x0 = -half + i * step;
            float x1 = -half + (i + 1) * step;
            float z0 = -half + j * step;
            float z1 = -half + (j + 1) * step;
            // Two triangles per cell with reversed vertex order for an upright plane.
            // First triangle: {x0, 0, z0}, {x1, 0, z1}, {x1, 0, z0}
            verts.push_back({x0, 0.0f, z0});
            verts.push_back({x1, 0.0f, z1});
            verts.push_back({x1, 0.0f, z0});
            // Second triangle: {x0, 0, z0}, {x0, 0, z1}, {x1, 0, z1}
            verts.push_back({x0, 0.0f, z0});
            verts.push_back({x0, 0.0f, z1});
            verts.push_back({x1, 0.0f, z1});
        }
    }
    return verts;
}


//-------------------------------------------------------------------------
// Sphere
//-------------------------------------------------------------------------
std::vector<Vertex> generateSphere(float radius, int slices, int stacks) {
    std::vector<Vertex> verts;
    float step_stacks = M_PI / stacks;
    float step_slices = 2 * M_PI / slices;
    for (int i = 0; i < stacks; i++) {
        float lat1 = i * step_stacks - M_PI/2;
        float lat2 = (i + 1) * step_stacks - M_PI/2;
        for (int j = 0; j < slices; j++) {
            float angle = j * step_slices;
            float angle2 = (j + 1) * step_slices;
            // First triangle.
            float x1 = radius * cosf(lat1) * cosf(angle);
            float y1 = radius * sinf(lat1);
            float z1 = radius * cosf(lat1) * sinf(angle);
            float x2 = radius * cosf(lat2) * cosf(angle);
            float y2 = radius * sinf(lat2);
            float z2 = radius * cosf(lat2) * sinf(angle);
            float x3 = radius * cosf(lat2) * cosf(angle2);
            float y3 = radius * sinf(lat2);
            float z3 = radius * cosf(lat2) * sinf(angle2);
            verts.push_back({x1, y1, z1});
            verts.push_back({x2, y2, z2});
            verts.push_back({x3, y3, z3});
            // Second triangle.
            float x6 = radius * cosf(lat1) * cosf(angle2);
            float y6 = radius * sinf(lat1);
            float z6 = radius * cosf(lat1) * sinf(angle2);
            verts.push_back({x1, y1, z1});
            verts.push_back({x3, y3, z3});
            verts.push_back({x6, y6, z6});
        }
    }
    return verts;
}

//-------------------------------------------------------------------------
// Cube
//-------------------------------------------------------------------------
std::vector<Vertex> generateCube(float dimension, int divisions) {
    std::vector<Vertex> verts;
    float half = dimension * 0.5f;
    float step = dimension / divisions;

    // Lambda functions for transforming the front face to the other five faces.
    auto front   = [](float x, float y, float z) -> Vertex { return {x, y, z}; };
    auto back    = [](float x, float y, float z) -> Vertex { return {-x, y, -z}; };
    auto right   = [](float x, float y, float z) -> Vertex { return {z, y, -x}; };
    auto left    = [](float x, float y, float z) -> Vertex { return {-z, y, x}; };
    auto top     = [](float x, float y, float z) -> Vertex { return {x, z, -y}; };
    auto bottom  = [](float x, float y, float z) -> Vertex { return {x, -z, y}; };

    typedef Vertex(*TransformFunc)(float, float, float);
    TransformFunc transforms[6] = { front, back, right, left, top, bottom };

    for (int t = 0; t < 6; t++) {
        TransformFunc trans = transforms[t];
        for (int i = 0; i < divisions; i++) {
            for (int j = 0; j < divisions; j++) {
                float x0 = -half + i * step;
                float x1 = -half + (i + 1) * step;
                float y0 = -half + j * step;
                float y1 = -half + (j + 1) * step;
                float z = half;
                Vertex v0 = trans(x0, y0, z);
                Vertex v1 = trans(x1, y0, z);
                Vertex v2 = trans(x1, y1, z);
                Vertex v3 = trans(x0, y1, z);
                // Two triangles per face patch.
                verts.push_back(v0);
                verts.push_back(v1);
                verts.push_back(v2);

                verts.push_back(v0);
                verts.push_back(v2);
                verts.push_back(v3);
            }
        }
    }
    return verts;
}

//-------------------------------------------------------------------------
// Cone
//-------------------------------------------------------------------------
std::vector<Vertex> generateCone(float bottomRadius, float height, int slices, int stacks) {
    std::vector<Vertex> verts;
    // Base of the cone.
    for (int j = 0; j < slices; j++){
        float angle0 = 2 * M_PI * j / slices;
        float angle1 = 2 * M_PI * (j + 1) / slices;
        verts.push_back({bottomRadius * cosf(angle1), 0.0f, bottomRadius * sinf(angle1)});
        verts.push_back({0.0f, 0.0f, 0.0f});
        verts.push_back({bottomRadius * cosf(angle0), 0.0f, bottomRadius * sinf(angle0)});
    }
    // Lateral surface.
    for (int i = 0; i < stacks; i++){
        float f0 = float(i) / stacks;
        float f1 = float(i + 1) / stacks;
        float y0 = f0 * height;
        float y1 = f1 * height;
        float r0 = bottomRadius * (1.0f - f0);
        float r1 = bottomRadius * (1.0f - f1);
        for (int j = 0; j < slices; j++){
            float angle0 = 2 * M_PI * j / slices;
            float angle1 = 2 * M_PI * (j + 1) / slices;
            float x0 = r0 * cosf(angle0), z0 = r0 * sinf(angle0);
            float x1 = r0 * cosf(angle1), z1 = r0 * sinf(angle1);
            float x2 = r1 * cosf(angle1), z2 = r1 * sinf(angle1);
            float x3 = r1 * cosf(angle0), z3 = r1 * sinf(angle0);
            if (i < stacks - 1) {
                verts.push_back({x0, y0, z0});
                verts.push_back({x2, y1, z2});
                verts.push_back({x1, y0, z1});

                verts.push_back({x0, y0, z0});
                verts.push_back({x3, y1, z3});
                verts.push_back({x2, y1, z2});
            } else {
                verts.push_back({x0, y0, z0});
                verts.push_back({0.0f, height, 0.0f});
                verts.push_back({x1, y0, z1});
            }
        }
    }
    return verts;
}

//-------------------------------------------------------------------------
// Write vertices to a .3d file in "models/generated/"
//-------------------------------------------------------------------------
void writeVertices(const std::vector<Vertex> &verts, const std::string &filename) {
    std::string outputPath = "../models/generated/" + filename;
    std::ofstream file(outputPath);
    if (!file.is_open()){
        std::cerr << "Error opening file: " << outputPath << std::endl;
        return;
    }
    // Write the number of vertices.
    file << verts.size() << "\n";
    for (const auto &v : verts)
        file << v[0] << " " << v[1] << " " << v[2] << "\n";
    file.close();
}
