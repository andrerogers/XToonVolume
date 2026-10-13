#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace svo {

struct Keyframe {
    float time;
    glm::vec3 translation;  // added to the joint's rest offset
    glm::quat rotation;
};

struct Joint {
    std::string name;
    int parent = -1;  // parents always precede children
    glm::vec3 offset{0.0f};  // rest position relative to the parent joint
    std::vector<Keyframe> keys;  // sorted by time; empty => rest pose
};

// A looping keyframed skeleton. Rotations are slerped, translations lerped,
// and the last key wraps around to the first at `duration`.
struct Skeleton {
    std::vector<Joint> joints;
    float duration = 1.0f;

    int addJoint(const std::string& name, int parent, glm::vec3 offset);
    // Euler angles in degrees, applied X then Y then Z.
    void addKey(int joint, float time, glm::vec3 eulerDegrees, glm::vec3 translation = glm::vec3(0.0f));

    // Joint-to-skeleton-root transforms for every joint at `time`.
    void pose(float time, std::vector<glm::mat4>& out) const;
};

}  // namespace svo
