#include "bezier.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <array>

// Small helper Vec3 for control points
struct Vec3 { float x, y, z; };

// Evaluate the Bernstein basis B_i^3(t)
static float bernstein3(int i, float t) {
    switch(i) {
        case 0: return (1 - t) * (1 - t) * (1 - t);
        case 1: return 3 * t * (1 - t) * (1 - t);
        case 2: return 3 * t * t * (1 - t);
        case 3: return t * t * t;
        default: return 0.0f;
    }
}


std::vector<Vertex> bezier(
    const std::string &controlPointFile,
    int tessellation)

{
    // 1) Read 16 control points from file into P[4][4]
    std::ifstream in(controlPointFile);
    if (!in) throw std::runtime_error("Cannot open control-point file: " + controlPointFile);

    std::array<std::array<Vec3,4>,4> P;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!(in >> P[i][j].x >> P[i][j].y >> P[i][j].z)) {
                throw std::runtime_error("Invalid control-point data in: " + controlPointFile);
            }
        }
    }

    // 2) Precompute grid of points on patch
    int n = tessellation;
    std::vector<Vec3> grid((n + 1) * (n + 1));
    for (int iu = 0; iu <= n; ++iu) {
        float u = float(iu) / n;
        for (int iv = 0; iv <= n; ++iv) {
            float v = float(iv) / n;
            // Evaluate S(u,v)
            float Bu[4], Bv[4];
            for (int k = 0; k < 4; ++k) {
                Bu[k] = bernstein3(k, u);
                Bv[k] = bernstein3(k, v);
            }
            Vec3 sum{0, 0, 0};
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    float b = Bu[i] * Bv[j];
                    sum.x += b * P[i][j].x;
                    sum.y += b * P[i][j].y;
                    sum.z += b * P[i][j].z;
                }
            }
            grid[iu * (n + 1) + iv] = sum;
        }
    }

    // 3) Generate triangles
    std::vector<Vertex> verts;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int idx = i * (n + 1) + j;
            auto v00 = grid[idx];
            auto v10 = grid[idx + 1];
            auto v01 = grid[idx + (n + 1)];
            auto v11 = grid[idx + (n + 1) + 1];
            // Triangle 1: (i,j) -> (i+1,j) -> (i,j+1)
            verts.push_back({v00.x, v00.y, v00.z});
            verts.push_back({v10.x, v10.y, v10.z});
            verts.push_back({v01.x, v01.y, v01.z});
            // Triangle 2: (i+1,j) -> (i+1,j+1) -> (i,j+1)
            verts.push_back({v10.x, v10.y, v10.z});
            verts.push_back({v11.x, v11.y, v11.z});
            verts.push_back({v01.x, v01.y, v01.z});
        }
    }

    return verts;
}
