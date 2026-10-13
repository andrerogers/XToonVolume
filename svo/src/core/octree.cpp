#include "octree.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace svo {

void VoxelGrid::resize(glm::ivec3 s)
{
    size = s;
    const size_t n = size_t(s.x) * s.y * s.z;
    occupied.assign(n, 0);
    color.assign(n, 0);
    normal.assign(n, glm::vec3(0.0f));
}

size_t VoxelGrid::countOccupied() const
{
    return size_t(std::count_if(occupied.begin(), occupied.end(), [](uint8_t o) { return o != 0; }));
}

uint32_t packColor(glm::vec4 c)
{
    c = glm::clamp(c, 0.0f, 1.0f) * 255.0f + 0.5f;
    return uint32_t(c.r) | (uint32_t(c.g) << 8) | (uint32_t(c.b) << 16) | (uint32_t(c.a) << 24);
}

glm::vec4 unpackColor(uint32_t c)
{
    return glm::vec4(float(c & 0xff), float((c >> 8) & 0xff), float((c >> 16) & 0xff), float(c >> 24)) / 255.0f;
}

static glm::vec2 signNotZero(glm::vec2 v)
{
    return glm::vec2(v.x >= 0.0f ? 1.0f : -1.0f, v.y >= 0.0f ? 1.0f : -1.0f);
}

uint32_t packNormal(glm::vec3 n)
{
    float len = std::abs(n.x) + std::abs(n.y) + std::abs(n.z);
    if (len == 0.0f)
        n = glm::vec3(0, 0, 1), len = 1.0f;
    glm::vec2 p = glm::vec2(n.x, n.y) / len;
    if (n.z < 0.0f)
        p = (1.0f - glm::abs(glm::vec2(p.y, p.x))) * signNotZero(p);
    auto q = [](float f) { return uint32_t(int32_t(std::lround(glm::clamp(f, -1.0f, 1.0f) * 32767.0f)) & 0xffff); };
    return q(p.x) | (q(p.y) << 16);
}

glm::vec3 unpackNormal(uint32_t p)
{
    auto u = [](uint32_t v) { return glm::max(float(int16_t(v & 0xffff)) / 32767.0f, -1.0f); };
    glm::vec2 e(u(p), u(p >> 16));
    glm::vec3 n(e.x, e.y, 1.0f - std::abs(e.x) - std::abs(e.y));
    if (n.z < 0.0f) {
        glm::vec2 xy = (1.0f - glm::abs(glm::vec2(n.y, n.x))) * signNotZero(glm::vec2(n.x, n.y));
        n.x = xy.x, n.y = xy.y;
    }
    return glm::normalize(n);
}

// Spreads the low 16 bits of v so there are two zero bits between each.
static uint64_t part1By2(uint64_t v)
{
    v &= 0xffff;
    v = (v | (v << 32)) & 0x001f00000000ffffull;
    v = (v | (v << 16)) & 0x001f0000ff0000ffull;
    v = (v | (v << 8)) & 0x100f00f00f00f00full;
    v = (v | (v << 4)) & 0x10c30c30c30c30c3ull;
    v = (v | (v << 2)) & 0x1249249249249249ull;
    return v;
}

static uint64_t morton(int x, int y, int z)
{
    return part1By2(uint64_t(x)) | (part1By2(uint64_t(y)) << 1) | (part1By2(uint64_t(z)) << 2);
}

namespace {
struct Cell {
    uint64_t code;
    glm::vec4 color;   // sum of leaf colours below
    glm::vec3 normal;  // sum of leaf normals below
    uint32_t count;    // number of leaf voxels below
    uint32_t mask;
    uint32_t firstChild;  // index into the next level
};
}  // namespace

Octree build(const VoxelGrid& grid)
{
    Octree tree;
    const int maxDim = std::max({grid.size.x, grid.size.y, grid.size.z, 1});
    while ((1 << tree.depth) < maxDim)
        ++tree.depth;

    // Leaf level, sorted by Morton code so siblings are adjacent.
    std::vector<std::vector<Cell>> levels(tree.depth + 1);
    auto& leaves = levels[tree.depth];
    leaves.reserve(grid.countOccupied());
    for (int z = 0; z < grid.size.z; ++z)
        for (int y = 0; y < grid.size.y; ++y)
            for (int x = 0; x < grid.size.x; ++x) {
                const size_t i = grid.index(x, y, z);
                if (!grid.occupied[i])
                    continue;
                leaves.push_back({morton(x, y, z), unpackColor(grid.color[i]), grid.normal[i], 1, 0, 0});
            }
    if (leaves.empty())
        return tree;
    std::sort(leaves.begin(), leaves.end(), [](const Cell& a, const Cell& b) { return a.code < b.code; });

    // Merge siblings into parents, level by level, up to the root.
    for (int level = tree.depth - 1; level >= 0; --level) {
        const auto& children = levels[level + 1];
        auto& parents = levels[level];
        for (uint32_t i = 0; i < children.size(); ++i) {
            const Cell& c = children[i];
            const uint64_t parentCode = c.code >> 3;
            if (parents.empty() || parents.back().code != parentCode)
                parents.push_back({parentCode, glm::vec4(0.0f), glm::vec3(0.0f), 0, 0, i});
            Cell& p = parents.back();
            p.color += c.color;
            p.normal += c.normal;
            p.count += c.count;
            p.mask |= 1u << (c.code & 7);
        }
    }

    // Flatten breadth-first: level L+1 starts right after level L.
    std::vector<uint32_t> offset(tree.depth + 2, 0);
    for (int level = 0; level <= tree.depth; ++level)
        offset[level + 1] = offset[level] + uint32_t(levels[level].size());
    tree.nodes.resize(offset[tree.depth + 1]);
    for (int level = 0; level <= tree.depth; ++level) {
        for (size_t i = 0; i < levels[level].size(); ++i) {
            const Cell& c = levels[level][i];
            Node& n = tree.nodes[offset[level] + i];
            n.childMask = c.mask;
            n.childBase = c.mask ? offset[level + 1] + c.firstChild : 0;
            n.color = packColor(c.color / float(c.count));
            const float len = glm::length(c.normal);
            n.normal = packNormal(len > 1e-6f ? c.normal / len : glm::vec3(0, 0, 1));
        }
    }
    return tree;
}

}  // namespace svo
