#pragma once

#include <string>

#include "animation.h"
#include "scene.h"

namespace svo {

struct DemoOptions {
    int walkers = 12;          // size of the crowd circling the centrepiece
    int partResolution = 64;   // voxels along the longest side of each limb
    int groundResolution = 512;
    // Optional raw volume for the centrepiece (e.g. the thesis bonsai.raw).
    // Without one, a procedural SDF tree is used.
    std::string volumePath;
    glm::ivec3 volumeSize{256};
    int volumeThreshold = 40;
};

// The demo world: a voxel ground plane, a static centrepiece and a crowd of
// articulated "walkers". Each walker is 10 rigid limb instances driven by one
// shared keyframed skeleton; every walker has its own phase, path and tint, but
// the whole crowd shares the same 6 limb octrees (instancing).
class Demo {
public:
    bool build(const DemoOptions& options, std::string& error);
    // Rewrites scene.instances for the given time (seconds).
    void update(float time);

    Scene scene;
    float speed = 1.0f;
    float centrepieceSpin = 0.1f;  // radians per second

private:
    struct Part {
        uint32_t model;
        int joint;
    };
    struct Walker {
        float radius, angle0, phase;
        glm::vec4 tint;
    };

    Skeleton skeleton_;
    std::vector<Part> parts_;
    std::vector<Walker> walkers_;
    uint32_t groundModel_ = 0, centreModel_ = 0;
    glm::mat4 centreBase_{1.0f};
    std::vector<glm::mat4> pose_;
};

}  // namespace svo
