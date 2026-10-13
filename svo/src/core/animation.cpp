#include "animation.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace svo {

int Skeleton::addJoint(const std::string& name, int parent, glm::vec3 offset)
{
    joints.push_back({name, parent, offset, {}});
    return int(joints.size()) - 1;
}

void Skeleton::addKey(int joint, float time, glm::vec3 eulerDegrees, glm::vec3 translation)
{
    const glm::vec3 r = glm::radians(eulerDegrees);
    const glm::quat q = glm::angleAxis(r.z, glm::vec3(0, 0, 1)) * glm::angleAxis(r.y, glm::vec3(0, 1, 0)) *
                        glm::angleAxis(r.x, glm::vec3(1, 0, 0));
    auto& keys = joints[joint].keys;
    keys.push_back({time, translation, q});
    std::sort(keys.begin(), keys.end(), [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
}

static void sample(const std::vector<Keyframe>& keys, float time, float duration, glm::vec3& t, glm::quat& q)
{
    if (keys.empty()) {
        t = glm::vec3(0.0f), q = glm::quat(1, 0, 0, 0);
        return;
    }
    if (keys.size() == 1) {
        t = keys[0].translation, q = keys[0].rotation;
        return;
    }
    // Find the bracketing keys, wrapping from the last key back to the first.
    size_t next = 0;
    while (next < keys.size() && keys[next].time <= time)
        ++next;
    const Keyframe& a = keys[(next + keys.size() - 1) % keys.size()];
    const Keyframe& b = keys[next % keys.size()];
    float span = b.time - a.time;
    float local = time - a.time;
    if (span <= 0.0f)
        span += duration;
    if (local < 0.0f)
        local += duration;
    const float f = span > 0.0f ? std::clamp(local / span, 0.0f, 1.0f) : 0.0f;
    t = glm::mix(a.translation, b.translation, f);
    q = glm::slerp(a.rotation, b.rotation, f);
}

void Skeleton::pose(float time, std::vector<glm::mat4>& out) const
{
    time = std::fmod(time, duration);
    if (time < 0.0f)
        time += duration;
    out.resize(joints.size());
    for (size_t i = 0; i < joints.size(); ++i) {
        glm::vec3 t;
        glm::quat q;
        sample(joints[i].keys, time, duration, t, q);
        const glm::mat4 local = glm::translate(glm::mat4(1.0f), joints[i].offset + t) * glm::mat4_cast(q);
        out[i] = joints[i].parent < 0 ? local : out[joints[i].parent] * local;
    }
}

}  // namespace svo
