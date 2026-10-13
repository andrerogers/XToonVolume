#pragma once

#include "image.h"
#include "scene.h"
#include "traverse.h"
#include "xtoon.h"

namespace svo {

struct Camera {
    glm::vec3 position{0.0f, 2.0f, 8.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f}, right{1.0f, 0.0f, 0.0f}, up{0.0f, 1.0f, 0.0f};
    float tanHalfFovY = 0.41421356f;  // 45 degrees
    float aspect = 16.0f / 9.0f;

    static Camera lookAt(glm::vec3 position, glm::vec3 target, float fovYDegrees, float aspect);
    // ndc in [-1, 1]^2, y up. Returns a unit direction.
    glm::vec3 rayDir(glm::vec2 ndc) const;
};

enum class View : int { XToon = 0, Albedo, Normal, Level, Cost, Detail };

struct RenderSettings {
    View view = View::XToon;
    xtoon::DetailMode detail = xtoon::DetailMode::Depth;
    float zMin = 6.0f, r = 5.0f;  // Depth: full detail up to zMin, none beyond r * zMin
    float focus = 10.0f, focusWidth = 1.5f;
    float backlightR = 2.0f;
    float highlightS = 24.0f;
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 0.8f, 0.35f));  // towards the light
    float albedoMix = 1.0f;       // 0 = toon texture only, 1 = tinted by voxel colour
    float lodBias = 1.0f;         // LOD cell footprint in pixels; 0 = always descend to leaves
    bool smoothNormals = true;    // prefiltered octree normals vs. blocky face normals
    bool shadows = true;
    bool outlines = true;
    float outlineDepth = 0.04f;   // relative depth jump that counts as an edge
    int maxSteps = 512;           // per instance
};

struct SceneHit {
    bool hit = false;
    float t = 0.0f;
    uint32_t instance = 0;
    Hit local;
    int steps = 0;  // summed over all instances visited
};

// Closest hit over all instances, through the top-level BVH. Each instance's ray is
// transformed to voxel space without renormalising, so t stays in world units and
// hits from different instances compare directly. pixelSpread is the angle subtended
// by one pixel (radians); it drives the LOD cut-off.
SceneHit traceScene(const FrameData& frame, const Node* nodes, glm::vec3 origin, glm::vec3 dir, float tMax,
                    float pixelSpread, const RenderSettings& settings, bool anyHit = false);

// Reference CPU renderer: the same algorithm and shading as shaders/render.comp.
// Used by svo_cli and to check the GPU path against.
Image renderCpu(const FrameData& frame, const std::vector<Node>& nodes, const Camera& camera, int width, int height,
                const RenderSettings& settings, const Image& toon, int threads = 0);

}  // namespace svo
