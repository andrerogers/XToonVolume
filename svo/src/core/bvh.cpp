#include "bvh.h"

#include <algorithm>

namespace svo {

Aabb transformAabb(const glm::mat4& m, glm::vec3 lo, glm::vec3 hi)
{
    Aabb box;
    for (int i = 0; i < 8; ++i) {
        const glm::vec3 corner(i & 1 ? hi.x : lo.x, i & 2 ? hi.y : lo.y, i & 4 ? hi.z : lo.z);
        box.grow(glm::vec3(m * glm::vec4(corner, 1.0f)));
    }
    return box;
}

float intersectAabb(glm::vec3 lo, glm::vec3 hi, glm::vec3 origin, glm::vec3 invDir, float tMax)
{
    const glm::vec3 t0 = (lo - origin) * invDir, t1 = (hi - origin) * invDir;
    const glm::vec3 tn = glm::min(t0, t1), tf = glm::max(t0, t1);
    const float enter = std::max({tn.x, tn.y, tn.z, 0.0f});
    const float exit = std::min({tf.x, tf.y, tf.z, tMax});
    return enter <= exit ? enter : -1.0f;
}

namespace {

struct Builder {
    const std::vector<Aabb>& boxes;
    Bvh& bvh;
    int maxLeaf;

    void build(uint32_t nodeIndex, uint32_t first, uint32_t count)
    {
        Aabb bounds, centroids;
        for (uint32_t i = first; i < first + count; ++i) {
            bounds.grow(boxes[bvh.order[i]]);
            centroids.grow(boxes[bvh.order[i]].center());
        }
        BvhNode& node = bvh.nodes[nodeIndex];
        node.lo = bounds.lo, node.hi = bounds.hi;
        if (count <= uint32_t(maxLeaf)) {
            node.first = first, node.count = count;
            return;
        }

        // Binned SAH over the centroid bounds.
        constexpr int kBins = 12;
        float bestCost = std::numeric_limits<float>::max();
        int bestAxis = -1, bestSplit = 0;
        for (int axis = 0; axis < 3; ++axis) {
            const float extent = centroids.hi[axis] - centroids.lo[axis];
            if (extent <= 0.0f)
                continue;
            Aabb binBox[kBins];
            int binCount[kBins] = {};
            for (uint32_t i = first; i < first + count; ++i) {
                const Aabb& b = boxes[bvh.order[i]];
                const int bin = std::min(kBins - 1, int((b.center()[axis] - centroids.lo[axis]) / extent * kBins));
                binBox[bin].grow(b);
                ++binCount[bin];
            }
            for (int split = 1; split < kBins; ++split) {
                Aabb left, right;
                int nl = 0, nr = 0;
                for (int b = 0; b < split; ++b)
                    if (binCount[b]) left.grow(binBox[b]), nl += binCount[b];
                for (int b = split; b < kBins; ++b)
                    if (binCount[b]) right.grow(binBox[b]), nr += binCount[b];
                if (!nl || !nr)
                    continue;
                const float cost = left.area() * nl + right.area() * nr;
                if (cost < bestCost)
                    bestCost = cost, bestAxis = axis, bestSplit = split;
            }
        }

        uint32_t mid;
        if (bestAxis < 0) {
            mid = first + count / 2;  // all centroids coincide: split the list in half
        } else {
            const float extent = centroids.hi[bestAxis] - centroids.lo[bestAxis];
            auto* begin = bvh.order.data() + first;
            auto* split = std::partition(begin, begin + count, [&](uint32_t item) {
                const float c = boxes[item].center()[bestAxis];
                return std::min(kBins - 1, int((c - centroids.lo[bestAxis]) / extent * kBins)) < bestSplit;
            });
            mid = uint32_t(split - bvh.order.data());
        }

        const uint32_t left = uint32_t(bvh.nodes.size());
        bvh.nodes.emplace_back();
        bvh.nodes.emplace_back();
        bvh.nodes[nodeIndex].first = left;  // `node` may dangle after emplace_back
        bvh.nodes[nodeIndex].count = 0;
        build(left, first, mid - first);
        build(left + 1, mid, first + count - mid);
    }
};

}  // namespace

Bvh buildBvh(const std::vector<Aabb>& boxes, int maxLeafSize)
{
    Bvh bvh;
    if (boxes.empty())
        return bvh;
    bvh.order.resize(boxes.size());
    for (uint32_t i = 0; i < boxes.size(); ++i)
        bvh.order[i] = i;
    bvh.nodes.reserve(boxes.size() * 2);
    bvh.nodes.emplace_back();
    Builder{boxes, bvh, std::max(1, maxLeafSize)}.build(0, 0, uint32_t(boxes.size()));
    return bvh;
}

}  // namespace svo
