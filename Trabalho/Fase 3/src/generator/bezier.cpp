#include "bezier.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <string>
#include <array>

// Helper struct for control points
struct Vec3 { float x, y, z; };

// Compute the Bernstein basis B_i^3(t)
static float bernstein3(int i, float t) {
    switch (i) {
        case 0: return (1 - t)*(1 - t)*(1 - t);
        case 1: return 3*t*(1 - t)*(1 - t);
        case 2: return 3*t*t*(1 - t);
        case 3: return t*t*t;
        default: return 0.0f;
    }
}

std::vector<Vertex> bezier(const std::string &patchFile, int tessellation) {
    std::ifstream in(patchFile);
    if (!in)
        throw std::runtime_error("Cannot open patch file: " + patchFile);

    std::string line;
    // Read number of patches
    if (!std::getline(in, line))
        throw std::runtime_error("Missing patch count in: " + patchFile);
    int numPatches = std::stoi(line);

    // Read patch index lists
    std::vector<std::array<int,16>> patches;
    patches.reserve(numPatches);
    for (int p = 0; p < numPatches; ++p) {
        if (!std::getline(in, line))
            throw std::runtime_error("Unexpected end of patch definitions in: " + patchFile);
        std::stringstream ss(line);
        std::array<int,16> idxs;
        for (int i = 0; i < 16; ++i) {
            std::string tok;
            if (!std::getline(ss, tok, ','))
                throw std::runtime_error("Invalid patch index data in: " + patchFile);
            idxs[i] = std::stoi(tok);
        }
        patches.push_back(idxs);
    }

    // Read number of control points
    if (!std::getline(in, line))
        throw std::runtime_error("Missing control-point count in: " + patchFile);
    int numPoints = std::stoi(line);

    // Read control points
    std::vector<Vec3> controlPoints;
    controlPoints.reserve(numPoints);
    for (int i = 0; i < numPoints; ++i) {
        if (!std::getline(in, line))
            throw std::runtime_error("Unexpected end of control-point data in: " + patchFile);
        std::stringstream ss(line);
        std::array<float,3> v;
        for (int k = 0; k < 3; ++k) {
            std::string tok;
            if (!std::getline(ss, tok, ','))
                throw std::runtime_error("Invalid control-point data in: " + patchFile);
            v[k] = std::stof(tok);
        }
        controlPoints.push_back({v[0], v[1], v[2]});
    }

    std::vector<Vertex> outVerts;
    outVerts.reserve((tessellation*tessellation*2) * 3 * numPatches);

    // Tessellate each patch
    for (auto &patch : patches) {
        // Build the 4x4 grid of control points for this patch
        std::array<std::array<Vec3,4>,4> P;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                P[i][j] = controlPoints[ patch[i*4 + j] ];

        // Evaluate grid points
        int n = tessellation;
        std::vector<Vec3> grid((n+1)*(n+1));
        for (int iu = 0; iu <= n; ++iu) {
            float u = float(iu)/n;
            float Bu[4]; for (int ii = 0; ii < 4; ++ii) Bu[ii] = bernstein3(ii, u);
            for (int iv = 0; iv <= n; ++iv) {
                float v = float(iv)/n;
                float Bv[4]; for (int jj = 0; jj < 4; ++jj) Bv[jj] = bernstein3(jj, v);
                Vec3 sum{0,0,0};
                for (int ii = 0; ii < 4; ++ii)
                    for (int jj = 0; jj < 4; ++jj) {
                        float b = Bu[ii]*Bv[jj];
                        sum.x += b*P[ii][jj].x;
                        sum.y += b*P[ii][jj].y;
                        sum.z += b*P[ii][jj].z;
                    }
                grid[iu*(n+1)+iv] = sum;
            }
        }
        
        // Create triangles
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                int idx = i*(n+1)+j;
                auto &v00 = grid[idx];
                auto &v10 = grid[idx+1];
                auto &v01 = grid[idx+(n+1)];
                auto &v11 = grid[idx+(n+1)+1];
                // Triangle 1
                outVerts.push_back({v00.x, v00.y, v00.z});
                outVerts.push_back({v10.x, v10.y, v10.z});
                outVerts.push_back({v01.x, v01.y, v01.z});
                // Triangle 2
                outVerts.push_back({v10.x, v10.y, v10.z});
                outVerts.push_back({v11.x, v11.y, v11.z});
                outVerts.push_back({v01.x, v01.y, v01.z});
            }
        }
    }

    return outVerts;
}
