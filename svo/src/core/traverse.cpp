#include "traverse.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace svo {

namespace {

// Avoids infinities in 1/d while keeping the sign of zero components positive.
glm::vec3 safeDir(glm::vec3 d)
{
    for (int a = 0; a < 3; ++a)
        if (std::abs(d[a]) < 1e-12f)
            d[a] = d[a] < 0.0f ? -1e-12f : 1e-12f;
    return d;
}

int argMin(glm::vec3 v) { return v.x <= v.y ? (v.x <= v.z ? 0 : 2) : (v.y <= v.z ? 1 : 2); }
int argMax(glm::vec3 v) { return v.x >= v.y ? (v.x >= v.z ? 0 : 2) : (v.y >= v.z ? 1 : 2); }

}  // namespace

Hit traverse(const Node* nodes, uint32_t root, int depth, const Ray& ray, float tMin, float tMax, float coneK, int maxSteps)
{
    Hit hit;
    const int res = 1 << depth;
    const glm::vec3 o = ray.origin;
    const glm::vec3 d = safeDir(ray.dir);
    const glm::vec3 invD = 1.0f / d;

    // Clip against the root cell [0, res]^3.
    const glm::vec3 t0 = -o * invD;
    const glm::vec3 t1 = (glm::vec3(float(res)) - o) * invD;
    const glm::vec3 tNear = glm::min(t0, t1), tFar = glm::max(t0, t1);
    float t = std::max({tNear.x, tNear.y, tNear.z, tMin});
    const float tExit = std::min({tFar.x, tFar.y, tFar.z, tMax});
    if (t > tExit)
        return hit;

    hit.axis = argMax(tNear);
    glm::ivec3 cell = glm::clamp(glm::ivec3(glm::floor(o + d * t)), glm::ivec3(0), glm::ivec3(res - 1));

    uint32_t stack[17];  // node index per level; depth <= 16
    stack[0] = root;
    int level = 0;

    for (int step = 0; step < maxSteps; ++step) {
        // Descend while the octant containing `cell` is occupied.
        int emptyLevel;
        for (;;) {
            const Node& n = nodes[stack[level]];
            const int size = res >> level;
            if (n.childMask == 0 || (coneK > 0.0f && float(size) <= coneK * t)) {
                hit.hit = true;
                hit.t = t;
                hit.level = level;
                hit.node = stack[level];
                hit.steps = step;
                return hit;
            }
            const int shift = depth - level - 1;
            const uint32_t oct = ((cell.x >> shift) & 1) | (((cell.y >> shift) & 1) << 1) | (((cell.z >> shift) & 1) << 2);
            if (!(n.childMask & (1u << oct))) {
                emptyLevel = level + 1;
                break;
            }
            stack[level + 1] = n.childBase + uint32_t(std::popcount(n.childMask & ((1u << oct) - 1u)));
            ++level;
        }

        // Skip the whole empty cell at emptyLevel: find which face the ray leaves through.
        const int shift = depth - emptyLevel;
        const int size = 1 << shift;
        const glm::ivec3 cellMin = (cell >> shift) << shift;
        const glm::vec3 bound = glm::vec3(cellMin) + glm::vec3(d.x > 0 ? size : 0, d.y > 0 ? size : 0, d.z > 0 ? size : 0);
        const glm::vec3 tm = (bound - o) * invD;
        const int axis = argMin(tm);
        t = std::max(t, tm[axis]);
        if (t > tExit)
            return hit;

        // Next leaf cell: step across the exit face, and keep the other axes inside the old cell.
        glm::ivec3 next = glm::clamp(glm::ivec3(glm::floor(o + d * t)), cellMin, cellMin + size - 1);
        next[axis] = d[axis] > 0 ? cellMin[axis] + size : cellMin[axis] - 1;
        if (next[axis] < 0 || next[axis] >= res)
            return hit;

        // Pop to the deepest level whose cell still contains `next`.
        const uint32_t diff = uint32_t((cell.x ^ next.x) | (cell.y ^ next.y) | (cell.z ^ next.z));
        const int highestBit = 31 - std::countl_zero(diff);
        level = std::min(level, depth - 1 - highestBit);
        cell = next;
        hit.axis = axis;
    }
    return hit;
}

bool contains(const Octree& tree, glm::ivec3 v)
{
    if (tree.empty() || glm::any(glm::lessThan(v, glm::ivec3(0))) ||
        glm::any(glm::greaterThanEqual(v, glm::ivec3(tree.resolution()))))
        return false;
    uint32_t node = 0;
    for (int level = 0; level < tree.depth; ++level) {
        const Node& n = tree.nodes[node];
        const int shift = tree.depth - level - 1;
        const uint32_t oct = ((v.x >> shift) & 1) | (((v.y >> shift) & 1) << 1) | (((v.z >> shift) & 1) << 2);
        if (!(n.childMask & (1u << oct)))
            return false;
        node = n.childBase + uint32_t(std::popcount(n.childMask & ((1u << oct) - 1u)));
    }
    return true;
}

Hit traverseReference(const Octree& tree, const Ray& ray, float tMin, float tMax)
{
    Hit hit;
    if (tree.empty())
        return hit;
    const int res = tree.resolution();
    const glm::vec3 o = ray.origin;
    const glm::vec3 d = safeDir(ray.dir);
    const glm::vec3 invD = 1.0f / d;
    const glm::vec3 t0 = -o * invD;
    const glm::vec3 t1 = (glm::vec3(float(res)) - o) * invD;
    const glm::vec3 tNear = glm::min(t0, t1), tFar = glm::max(t0, t1);
    float t = std::max({tNear.x, tNear.y, tNear.z, tMin});
    const float tExit = std::min({tFar.x, tFar.y, tFar.z, tMax});
    if (t > tExit)
        return hit;

    glm::ivec3 cell = glm::clamp(glm::ivec3(glm::floor(o + d * t)), glm::ivec3(0), glm::ivec3(res - 1));
    const glm::ivec3 stepDir(d.x > 0 ? 1 : -1, d.y > 0 ? 1 : -1, d.z > 0 ? 1 : -1);
    for (;;) {
        if (contains(tree, cell)) {
            hit.hit = true;
            hit.t = t;
            hit.level = tree.depth;
            return hit;
        }
        const glm::vec3 bound = glm::vec3(cell) + glm::vec3(d.x > 0 ? 1 : 0, d.y > 0 ? 1 : 0, d.z > 0 ? 1 : 0);
        const glm::vec3 tm = (bound - o) * invD;
        const int axis = argMin(tm);
        t = std::max(t, tm[axis]);
        if (t > tExit)
            return hit;
        cell[axis] += stepDir[axis];
        if (cell[axis] < 0 || cell[axis] >= res)
            return hit;
    }
}

}  // namespace svo
