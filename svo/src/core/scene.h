#pragma once

#include <string>
#include <vector>

#include "bvh.h"
#include "octree.h"

namespace svo {

// A voxel model: one octree in its own voxel space [0, res]^3.
// Many instances can share one model (the whole crowd shares 6 limb octrees).
struct Model {
    std::string name;
    Octree tree;
    glm::mat4 voxelToObject{1.0f};  // voxel space -> the model's object space
    uint32_t nodeOffset = 0;        // first node in the packed buffer, set by Scene::packNodes
};

struct Instance {
    uint32_t model = 0;
    glm::mat4 objectToWorld{1.0f};
    glm::vec4 tint{1.0f};
};

// Per-instance data as the shader sees it (std430, 144 bytes).
struct GpuInstance {
    glm::mat4 worldToVoxel;
    glm::mat4 voxelToWorld;
    uint32_t rootNode;
    uint32_t depth;
    float voxelSize;  // world-space edge length of one leaf voxel (uniform scale assumed)
    uint32_t tint;    // RGBA8
};
static_assert(sizeof(GpuInstance) == 144, "GpuInstance must match the GLSL layout");

// Everything the renderers need for one frame: instances in BVH leaf order plus the BVH.
struct FrameData {
    std::vector<GpuInstance> instances;
    std::vector<BvhNode> bvh;
};

struct Scene {
    std::vector<Model> models;
    std::vector<Instance> instances;

    uint32_t addModel(Model m);
    // Concatenates every model's nodes, rewriting child indices to be absolute.
    std::vector<Node> packNodes();
    // Builds the top-level BVH over the current instance transforms.
    FrameData prepareFrame() const;
    size_t totalNodes() const;
};

}  // namespace svo
