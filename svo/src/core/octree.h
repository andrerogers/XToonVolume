#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace svo {

// One octree node. 16 bytes, uploaded to the GPU unchanged as a std430 uvec4.
//
// Children of a node are stored contiguously and only for non-empty octants,
// so the address of child `oct` is childBase + popcount(childMask & ((1 << oct) - 1)).
// Octant index is x | (y << 1) | (z << 2).
//
// Every node (not just leaves) carries a prefiltered colour and normal so the
// traversal can stop early when a cell projects to less than a pixel (LOD).
struct Node {
    uint32_t childBase = 0;  // index of the first child in the node array
    uint32_t childMask = 0;  // bits 0-7: occupied octants. 0 => leaf
    uint32_t color = 0;      // RGBA8, averaged over the subtree
    uint32_t normal = 0;     // octahedral-encoded unit normal, snorm16x2
};
static_assert(sizeof(Node) == 16, "Node must match the GLSL uvec4 layout");

// Dense voxel input to the builder. Only cells with occupied != 0 are read.
struct VoxelGrid {
    glm::ivec3 size{0};
    std::vector<uint8_t> occupied;
    std::vector<uint32_t> color;    // RGBA8
    std::vector<glm::vec3> normal;  // outward unit normal

    void resize(glm::ivec3 s);
    size_t index(int x, int y, int z) const { return (size_t(z) * size.y + y) * size.x + x; }
    size_t countOccupied() const;
};

struct Octree {
    int depth = 0;            // leaves live at this level; resolution = 1 << depth
    std::vector<Node> nodes;  // breadth-first; nodes[0] is the root

    int resolution() const { return 1 << depth; }
    bool empty() const { return nodes.empty(); }
    size_t bytes() const { return nodes.size() * sizeof(Node); }
};

// Builds a sparse voxel octree bottom-up from Morton-sorted voxels.
// Depth is the smallest power of two that covers the largest grid dimension.
Octree build(const VoxelGrid& grid);

// Packing helpers shared with the shaders (see shaders/svo.glsl).
uint32_t packColor(glm::vec4 c);
glm::vec4 unpackColor(uint32_t c);
uint32_t packNormal(glm::vec3 n);    // octahedral, snorm16x2
glm::vec3 unpackNormal(uint32_t p);

}  // namespace svo
