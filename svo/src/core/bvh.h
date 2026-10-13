#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include <glm/glm.hpp>

namespace svo {

struct Aabb {
    glm::vec3 lo{std::numeric_limits<float>::max()};
    glm::vec3 hi{-std::numeric_limits<float>::max()};

    void grow(glm::vec3 p) { lo = glm::min(lo, p), hi = glm::max(hi, p); }
    void grow(const Aabb& b) { lo = glm::min(lo, b.lo), hi = glm::max(hi, b.hi); }
    glm::vec3 center() const { return (lo + hi) * 0.5f; }
    float area() const
    {
        const glm::vec3 e = glm::max(hi - lo, glm::vec3(0.0f));
        return 2.0f * (e.x * e.y + e.y * e.z + e.z * e.x);
    }
    bool valid() const { return lo.x <= hi.x; }
};

// World-space AABB of the box [lo, hi] after an affine transform.
Aabb transformAabb(const glm::mat4& m, glm::vec3 lo, glm::vec3 hi);

// 32 bytes, matches the std430 struct in shaders/svo.glsl.
// Leaf when count > 0: items order[first .. first + count). Otherwise children are
// nodes[first] and nodes[first + 1].
struct BvhNode {
    glm::vec3 lo;
    uint32_t first;
    glm::vec3 hi;
    uint32_t count;
};
static_assert(sizeof(BvhNode) == 32, "BvhNode must match the GLSL layout");

struct Bvh {
    std::vector<BvhNode> nodes;
    std::vector<uint32_t> order;  // leaf item indices, in leaf order
};

// Top-level BVH over instance boxes, binned SAH. Rebuilt every frame for animated
// instances; with a few hundred instances this takes well under a millisecond.
Bvh buildBvh(const std::vector<Aabb>& boxes, int maxLeafSize = 2);

// Slab test; returns the entry distance or a negative value on a miss.
float intersectAabb(glm::vec3 lo, glm::vec3 hi, glm::vec3 origin, glm::vec3 invDir, float tMax);

}  // namespace svo
