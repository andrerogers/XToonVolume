#include "voxelize.h"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace svo {

void keepShell(VoxelGrid& grid)
{
    const glm::ivec3 s = grid.size;
    auto solid = [&](int x, int y, int z) {
        if (x < 0 || y < 0 || z < 0 || x >= s.x || y >= s.y || z >= s.z)
            return false;
        return grid.occupied[grid.index(x, y, z)] != 0;
    };
    std::vector<uint8_t> keep(grid.occupied.size(), 0);
    for (int z = 0; z < s.z; ++z)
        for (int y = 0; y < s.y; ++y)
            for (int x = 0; x < s.x; ++x) {
                if (!solid(x, y, z))
                    continue;
                const bool interior = solid(x - 1, y, z) && solid(x + 1, y, z) && solid(x, y - 1, z) &&
                                      solid(x, y + 1, z) && solid(x, y, z - 1) && solid(x, y, z + 1);
                keep[grid.index(x, y, z)] = interior ? 0 : 1;
            }
    grid.occupied.swap(keep);
}

SdfVoxelization voxelizeSdf(const Sdf& sdf, glm::vec3 boundsMin, glm::vec3 boundsMax, int resolution,
                            const std::function<glm::vec4(glm::vec3)>& color, bool shellOnly)
{
    SdfVoxelization out;
    const glm::vec3 extent = boundsMax - boundsMin;
    out.voxelSize = std::max({extent.x, extent.y, extent.z}) / float(resolution);
    out.origin = boundsMin;
    const glm::ivec3 size = glm::max(glm::ivec3(glm::ceil(extent / out.voxelSize)), glm::ivec3(1));
    out.grid.resize(size);

    const float h = out.voxelSize * 0.5f;
    for (int z = 0; z < size.z; ++z)
        for (int y = 0; y < size.y; ++y)
            for (int x = 0; x < size.x; ++x) {
                const glm::vec3 p = boundsMin + (glm::vec3(x, y, z) + 0.5f) * out.voxelSize;
                if (sdf(p) > 0.0f)
                    continue;
                const size_t i = out.grid.index(x, y, z);
                out.grid.occupied[i] = 1;
                const glm::vec3 g(sdf(p + glm::vec3(h, 0, 0)) - sdf(p - glm::vec3(h, 0, 0)),
                                  sdf(p + glm::vec3(0, h, 0)) - sdf(p - glm::vec3(0, h, 0)),
                                  sdf(p + glm::vec3(0, 0, h)) - sdf(p - glm::vec3(0, 0, h)));
                const float len = glm::length(g);
                out.grid.normal[i] = len > 1e-8f ? g / len : glm::vec3(0, 1, 0);
                out.grid.color[i] = packColor(color(p));
            }
    if (shellOnly)
        keepShell(out.grid);
    return out;
}

bool loadRaw(const std::string& path, glm::ivec3 size, RawVolume& out, std::string& error)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        error = "cannot open " + path;
        return false;
    }
    const size_t expected = size_t(size.x) * size.y * size.z;
    const size_t actual = size_t(f.tellg());
    if (actual != expected) {
        error = path + " is " + std::to_string(actual) + " bytes but " + std::to_string(size.x) + "x" +
                std::to_string(size.y) + "x" + std::to_string(size.z) + " needs " + std::to_string(expected);
        return false;
    }
    f.seekg(0);
    out.size = size;
    out.data.resize(expected);
    f.read(reinterpret_cast<char*>(out.data.data()), std::streamsize(expected));
    return bool(f);
}

VoxelGrid voxelizeVolume(const RawVolume& v, uint8_t threshold, const std::function<glm::vec4(float)>& ramp,
                         bool shellOnly)
{
    VoxelGrid grid;
    grid.resize(v.size);
    auto sample = [&](int x, int y, int z) {
        x = std::clamp(x, 0, v.size.x - 1);
        y = std::clamp(y, 0, v.size.y - 1);
        z = std::clamp(z, 0, v.size.z - 1);
        return float(v.at(x, y, z));
    };
    for (int z = 0; z < v.size.z; ++z)
        for (int y = 0; y < v.size.y; ++y)
            for (int x = 0; x < v.size.x; ++x) {
                const uint8_t density = v.at(x, y, z);
                if (density < threshold)
                    continue;
                const size_t i = grid.index(x, y, z);
                grid.occupied[i] = 1;
                // Signed gradient points towards higher density; the surface normal is its negation.
                const glm::vec3 g(sample(x + 1, y, z) - sample(x - 1, y, z), sample(x, y + 1, z) - sample(x, y - 1, z),
                                  sample(x, y, z + 1) - sample(x, y, z - 1));
                const float len = glm::length(g);
                grid.normal[i] = len > 1e-6f ? -g / len : glm::vec3(0, 1, 0);
                grid.color[i] = packColor(ramp(float(density) / 255.0f));
            }
    if (shellOnly)
        keepShell(grid);
    return grid;
}

}  // namespace svo
