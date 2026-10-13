#pragma once

#include "octree.h"

namespace svo {

struct Ray {
    glm::vec3 origin;
    glm::vec3 dir;  // need not be normalised; t is measured in units of |dir|
};

struct Hit {
    bool hit = false;
    float t = 0.0f;       // ray parameter at entry into the hit cell
    int level = 0;        // octree level of the hit cell (depth for a leaf, less when LOD stopped early)
    uint32_t node = 0;    // index of the hit node
    int axis = 0;         // axis of the face the ray entered through (for blocky normals)
    int steps = 0;        // number of empty cells skipped (cost metric)
};

// Closest-hit traversal of one octree in its local voxel space, where the root
// cell spans [0, resolution]^3.
//
// This is a hierarchical DDA: descend through occupied children until a leaf,
// or until an empty octant is found; skip that whole empty cell in one step,
// then pop only as many levels as the step crossed (found from the highest
// differing bit of the old and new integer cell coordinates). The stack only
// holds node indices; cell positions come from the integer coordinates.
//
// coneK > 0 enables level-of-detail: a cell of size s (voxels) at ray parameter
// t counts as a hit once s <= coneK * t, i.e. when it is smaller than the pixel
// footprint. The shader in shaders/svo.glsl is a line-by-line port of this.
Hit traverse(const Node* nodes, uint32_t root, int depth, const Ray& ray, float tMin, float tMax, float coneK = 0.0f,
             int maxSteps = 1024);

// Brute-force reference: Amanatides-Woo DDA over every leaf voxel. Only used by tests.
Hit traverseReference(const Octree& tree, const Ray& ray, float tMin, float tMax);

// Returns true if the leaf voxel (x, y, z) exists in the tree.
bool contains(const Octree& tree, glm::ivec3 voxel);

}  // namespace svo
