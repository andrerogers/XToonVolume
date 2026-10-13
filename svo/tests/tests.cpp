// Unit tests for the core library. No framework: each test is a function, and
// CHECK records failures without stopping, so one run shows everything broken.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <random>
#include <set>
#include <tuple>

#include <glm/gtc/matrix_transform.hpp>

#include "animation.h"
#include "bvh.h"
#include "image.h"
#include "octree.h"
#include "render.h"
#include "traverse.h"
#include "voxelize.h"
#include "xtoon.h"

using namespace svo;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_checks;                                                                   \
        if (!(cond)) {                                                                \
            ++g_failures;                                                             \
            std::printf("  FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
        }                                                                             \
    } while (0)

#define CHECK_NEAR(a, b, eps) CHECK(std::abs(double(a) - double(b)) <= double(eps))

static VoxelGrid randomGrid(glm::ivec3 size, float fill, uint32_t seed)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> u(0.0f, 1.0f);
    VoxelGrid g;
    g.resize(size);
    for (size_t i = 0; i < g.occupied.size(); ++i)
        if (u(rng) < fill) {
            g.occupied[i] = 1;
            g.color[i] = packColor(glm::vec4(u(rng), u(rng), u(rng), 1.0f));
            g.normal[i] = glm::normalize(glm::vec3(u(rng) - 0.5f, u(rng) - 0.5f, u(rng) - 0.5f) + 1e-3f);
        }
    return g;
}

static Ray randomRay(std::mt19937& rng, float extent, bool inside)
{
    std::uniform_real_distribution<float> u(0.0f, 1.0f);
    Ray r;
    if (inside) {
        r.origin = glm::vec3(u(rng), u(rng), u(rng)) * extent;
    } else {
        // On a sphere around the volume, aimed roughly at it.
        const glm::vec3 c(extent * 0.5f);
        glm::vec3 dir = glm::normalize(glm::vec3(u(rng) - 0.5f, u(rng) - 0.5f, u(rng) - 0.5f) + 1e-4f);
        r.origin = c + dir * extent * 1.5f;
    }
    const glm::vec3 target = glm::vec3(u(rng), u(rng), u(rng)) * extent;
    r.dir = target - r.origin;
    if (glm::length(r.dir) < 1e-3f)
        r.dir = glm::vec3(1, 0.3f, 0.2f);
    // Some rays are exactly axis aligned or diagonal, the classic DDA edge cases.
    const float pick = u(rng);
    if (pick < 0.05f) r.dir = glm::vec3(0, 0, r.dir.z < 0 ? -1 : 1);
    else if (pick < 0.1f) r.dir = glm::vec3(1, 1, 1) * (r.dir.x < 0 ? -1.0f : 1.0f);
    else if (pick < 0.15f) r.dir = glm::vec3(r.dir.x, 0, r.dir.z);
    return r;
}

static void testPacking()
{
    for (int i = 0; i < 1000; ++i) {
        const float a = float(i) * 0.731f, b = float(i) * 1.37f;
        const glm::vec3 n(std::sin(a) * std::cos(b), std::sin(a) * std::sin(b), std::cos(a));
        CHECK(glm::length(unpackNormal(packNormal(n)) - n) < 1e-3f);
    }
    CHECK(glm::length(unpackNormal(packNormal(glm::vec3(0, 0, -1))) - glm::vec3(0, 0, -1)) < 1e-4f);
    const glm::vec4 c(0.1f, 0.5f, 0.9f, 1.0f);
    CHECK(glm::length(unpackColor(packColor(c)) - c) < 5e-3f);  // half a step per channel
}

static void testBuild()
{
    const VoxelGrid g = randomGrid({37, 50, 29}, 0.03f, 1);
    const Octree t = build(g);
    CHECK(t.depth == 6);  // 50 rounds up to 64

    // Every occupied voxel is in the tree and nothing else is.
    size_t occupied = 0, mismatches = 0;
    for (int z = 0; z < 64; ++z)
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                const bool in = x < g.size.x && y < g.size.y && z < g.size.z && g.occupied[g.index(x, y, z)];
                occupied += in;
                mismatches += in != contains(t, {x, y, z});
            }
    CHECK(mismatches == 0);

    // Node count equals the number of distinct occupied cells over all levels.
    size_t expected = 0;
    for (int level = 0; level <= t.depth; ++level) {
        std::set<std::tuple<int, int, int>> cells;
        const int shift = t.depth - level;
        for (int z = 0; z < g.size.z; ++z)
            for (int y = 0; y < g.size.y; ++y)
                for (int x = 0; x < g.size.x; ++x)
                    if (g.occupied[g.index(x, y, z)])
                        cells.insert({x >> shift, y >> shift, z >> shift});
        expected += cells.size();
    }
    CHECK(t.nodes.size() == expected);

    // The root colour is the average of every leaf colour (prefiltering).
    glm::vec4 sum(0.0f);
    for (size_t i = 0; i < g.occupied.size(); ++i)
        if (g.occupied[i])
            sum += unpackColor(g.color[i]);
    CHECK(glm::length(unpackColor(t.nodes[0].color) - sum / float(occupied)) < 0.01f);

    CHECK(build(VoxelGrid{}).empty());
}

static void testTraversalMatchesReference()
{
    std::mt19937 rng(7);
    int hits = 0;
    for (int scene = 0; scene < 4; ++scene) {
        const glm::ivec3 size = scene == 0 ? glm::ivec3(64) : glm::ivec3(20 + scene * 13, 50 - scene * 7, 33);
        const VoxelGrid g = randomGrid(size, scene == 3 ? 0.0005f : 0.01f, 100 + scene);
        const Octree t = build(g);
        int mismatches = 0;
        for (int i = 0; i < 4000; ++i) {
            const Ray r = randomRay(rng, float(t.resolution()), i % 4 == 0);
            const Hit a = traverse(t.nodes.data(), 0, t.depth, r, 0.0f, 1e30f, 0.0f, 1 << 20);
            const Hit b = traverseReference(t, r, 0.0f, 1e30f);
            hits += b.hit;
            const bool ok = a.hit == b.hit && (!a.hit || std::abs(a.t - b.t) <= 1e-3f * std::max(1.0f, b.t));
            if (!ok && mismatches++ < 3)
                std::printf("  ray o=(%g,%g,%g) d=(%g,%g,%g): octree %d t=%g, reference %d t=%g\n", r.origin.x,
                            r.origin.y, r.origin.z, r.dir.x, r.dir.y, r.dir.z, a.hit, a.t, b.hit, b.t);
        }
        CHECK(mismatches == 0);
    }
    CHECK(hits > 1000);  // the test must exercise hits, not only misses
}

static void testLod()
{
    const VoxelGrid g = randomGrid({64, 64, 64}, 0.02f, 3);
    const Octree t = build(g);
    std::mt19937 rng(9);
    int worse = 0, coarser = 0;
    for (int i = 0; i < 2000; ++i) {
        const Ray r = randomRay(rng, 64.0f, false);
        const Hit full = traverse(t.nodes.data(), 0, t.depth, r, 0.0f, 1e30f, 0.0f, 1 << 20);
        const Hit lod = traverse(t.nodes.data(), 0, t.depth, r, 0.0f, 1e30f, 0.05f, 1 << 20);
        // A coarse cell contains its fine cells, so LOD can only hit earlier, never miss more.
        if (full.hit && (!lod.hit || lod.t > full.t + 1e-3f))
            ++worse;
        coarser += lod.hit && lod.level < t.depth;
    }
    CHECK(worse == 0);
    CHECK(coarser > 0);
    // A huge cone stops at the root.
    const Hit root = traverse(t.nodes.data(), 0, t.depth, {glm::vec3(-10, 32, 32), glm::vec3(1, 0, 0)}, 0, 1e30f, 1e6f);
    CHECK(root.hit && root.level == 0);
    CHECK_NEAR(root.t, 10.0f, 1e-4f);
}

static void testBvh()
{
    std::mt19937 rng(11);
    std::uniform_real_distribution<float> u(-20.0f, 20.0f), s(0.2f, 3.0f);
    std::vector<Aabb> boxes(300);
    for (auto& b : boxes) {
        const glm::vec3 c(u(rng), u(rng), u(rng));
        b.lo = c - glm::vec3(s(rng), s(rng), s(rng));
        b.hi = c + glm::vec3(s(rng), s(rng), s(rng));
    }
    const Bvh bvh = buildBvh(boxes);
    std::vector<int> seen(boxes.size(), 0);
    for (uint32_t i : bvh.order)
        ++seen[i];
    CHECK(std::count(seen.begin(), seen.end(), 1) == int(boxes.size()));

    // Every leaf's bounds contain its items; every interior's bounds contain its children.
    bool contained = true;
    auto inside = [](const Aabb& b, glm::vec3 lo, glm::vec3 hi) {
        return glm::all(glm::greaterThanEqual(b.lo, lo)) && glm::all(glm::lessThanEqual(b.hi, hi));
    };
    for (const BvhNode& n : bvh.nodes) {
        if (n.count)
            for (uint32_t i = n.first; i < n.first + n.count; ++i)
                contained &= inside(boxes[bvh.order[i]], n.lo, n.hi);
        else
            for (int c = 0; c < 2; ++c)
                contained &= inside(Aabb{bvh.nodes[n.first + c].lo, bvh.nodes[n.first + c].hi}, n.lo, n.hi);
    }
    CHECK(contained);

    // Closest box through the BVH equals brute force.
    int mismatches = 0;
    for (int i = 0; i < 2000; ++i) {
        const glm::vec3 o(u(rng) * 2.0f, u(rng) * 2.0f, u(rng) * 2.0f);
        const glm::vec3 d = glm::normalize(glm::vec3(u(rng), u(rng), u(rng)) - o);
        const glm::vec3 inv = 1.0f / d;
        float brute = 1e30f;
        for (const auto& b : boxes) {
            const float t = intersectAabb(b.lo, b.hi, o, inv, 1e30f);
            if (t >= 0.0f) brute = std::min(brute, t);
        }
        float best = 1e30f;
        std::vector<uint32_t> stack = {0};
        while (!stack.empty()) {
            const BvhNode n = bvh.nodes[stack.back()];
            stack.pop_back();
            if (intersectAabb(n.lo, n.hi, o, inv, best) < 0.0f) continue;
            if (n.count) {
                for (uint32_t k = n.first; k < n.first + n.count; ++k) {
                    const float t = intersectAabb(boxes[bvh.order[k]].lo, boxes[bvh.order[k]].hi, o, inv, best);
                    if (t >= 0.0f) best = std::min(best, t);
                }
            } else {
                stack.push_back(n.first), stack.push_back(n.first + 1);
            }
        }
        mismatches += std::abs(best - brute) > 1e-4f;
    }
    CHECK(mismatches == 0);
}

static void testSceneTrace()
{
    // A single solid 8^3 cube model, instanced with rotation and scale.
    VoxelGrid g;
    g.resize({8, 8, 8});
    std::fill(g.occupied.begin(), g.occupied.end(), 1);
    std::fill(g.normal.begin(), g.normal.end(), glm::vec3(0, 1, 0));
    Scene scene;
    Model m;
    m.tree = build(g);
    m.voxelToObject = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f));  // 8 voxels -> 2 m cube at [0, 2]^3
    scene.addModel(m);
    const std::vector<Node> nodes = scene.packNodes();

    // Instance at x in [5, 7]: a ray from the origin along +x enters at t = 5 (world units).
    scene.instances.push_back({0, glm::translate(glm::mat4(1.0f), glm::vec3(5, -1, -1)), glm::vec4(1)});
    RenderSettings rs;
    rs.lodBias = 0.0f;
    FrameData frame = scene.prepareFrame();
    SceneHit h = traceScene(frame, nodes.data(), glm::vec3(0), glm::vec3(1, 0, 0), 1e30f, 0.0f, rs);
    CHECK(h.hit);
    CHECK_NEAR(h.t, 5.0f, 1e-4f);

    // Add a nearer, rotated and scaled-up instance; the closest hit must switch to it.
    glm::mat4 near = glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 0, 0)) *
                     glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 0, 1)) *
                     glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-1));
    scene.instances.push_back({0, near, glm::vec4(1)});
    frame = scene.prepareFrame();
    h = traceScene(frame, nodes.data(), glm::vec3(0), glm::vec3(1, 0, 0), 1e30f, 0.0f, rs);
    // A 1 m cube rotated 45 degrees about z: its left corner is at 2.5 - sqrt(0.5).
    CHECK(h.hit);
    CHECK_NEAR(h.t, 2.5f - std::sqrt(0.5f), 0.02f);
    CHECK(traceScene(frame, nodes.data(), glm::vec3(0), glm::vec3(-1, 0, 0), 1e30f, 0.0f, rs).hit == false);
}

static void testAnimation()
{
    Skeleton s;
    const int root = s.addJoint("root", -1, {0, 1, 0});
    const int child = s.addJoint("child", root, {0, -1, 0});
    s.duration = 2.0f;
    s.addKey(root, 0.0f, {0, 0, 0});
    s.addKey(root, 1.0f, {0, 0, 90});
    std::vector<glm::mat4> pose;

    s.pose(1.0f, pose);  // exactly on the second key: 90 degrees about z
    glm::vec3 p = glm::vec3(pose[child] * glm::vec4(0, 0, 0, 1));
    CHECK(glm::length(p - glm::vec3(1, 1, 0)) < 1e-4f);

    s.pose(0.5f, pose);  // halfway: 45 degrees
    p = glm::vec3(pose[child] * glm::vec4(0, 0, 0, 1));
    CHECK(glm::length(p - glm::vec3(std::sqrt(0.5f), 1 - std::sqrt(0.5f), 0)) < 1e-4f);

    s.pose(1.5f, pose);  // wrapping from the last key back to the first: 45 degrees again
    const glm::vec3 wrapped = glm::vec3(pose[child] * glm::vec4(0, 0, 0, 1));
    CHECK(glm::length(wrapped - p) < 1e-4f);

    s.pose(4.0f, pose);  // loops: same as t = 0
    p = glm::vec3(pose[child] * glm::vec4(0, 0, 0, 1));
    CHECK(glm::length(p - glm::vec3(0, 0, 0)) < 1e-4f);
}

static void testXToon()
{
    using namespace xtoon;
    bool finite = true, inRange = true;
    for (float r : {1.0f, 1.0001f, 2.0f, 10.0f})
        for (float z = 0.0f; z < 100.0f; z += 0.37f) {
            for (float d : {detailDepth(z, 3.0f, r), detailFocus(z, 10.0f, 1.0f, r), detailBacklight(z / 50 - 1, r),
                            detailHighlight(z / 50 - 1, r)}) {
                finite &= std::isfinite(d);
                inRange &= d >= 0.0f && d <= 1.0f;
            }
        }
    CHECK(finite);
    CHECK(inRange);
    CHECK(detailDepth(2.0f, 3.0f, 4.0f) == 1.0f);       // closer than zMin: full detail
    CHECK_NEAR(detailDepth(12.0f, 3.0f, 4.0f), 0.0f, 1e-6f);  // at zMax = r * zMin: none
    CHECK_NEAR(detailDepth(6.0f, 3.0f, 4.0f), 0.5f, 1e-6f);   // log-halfway
    CHECK(detailBacklight(-1.0f, 2.0f) == 1.0f);        // |n.v|: back-facing normals are fine
    CHECK(detailLevel(3, 6) == 0.5f);
}

static void testImages()
{
    Image toon;
    std::string error;
    CHECK(loadTga(std::string(SVO_ASSET_DIR) + "/toon/transferTex_01.tga", toon, error));
    CHECK(toon.width == 256 && toon.height == 256);
    const Image bands = makeToonTexture(ToonPreset::Bands, 64);
    // Detail 1, tone 0 is the darkest band; detail 0 is flat.
    CHECK(bands.sample({0.0f, 1.0f}).r < 0.4f);
    CHECK_NEAR(bands.sample({0.0f, 0.0f}).r, bands.sample({1.0f, 0.0f}).r, 0.03f);
}

int main()
{
    const std::pair<const char*, std::function<void()>> tests[] = {
        {"packing", testPacking},         {"build", testBuild},
        {"traversal", testTraversalMatchesReference},
        {"lod", testLod},                 {"bvh", testBvh},
        {"scene trace", testSceneTrace},  {"animation", testAnimation},
        {"xtoon", testXToon},             {"images", testImages},
    };
    for (const auto& [name, fn] : tests) {
        const int before = g_failures;
        fn();
        std::printf("%-12s %s\n", name, g_failures == before ? "ok" : "FAILED");
    }
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
