#pragma once
#include <vector>
#include <string>
#include "primitives.h"  // for Vertex typedef

/// Tessellates a 4×4 Bézier patch defined by controlPointFile (16 rows of x y z),
/// at the given tessellation level, and returns a list of triangles.
  
std::vector<Vertex> bezier(
    const std::string &controlPointFile,
    int tessellation);
    