#pragma once

#include <functional>
#include <string>

#include "octree.h"

namespace svo {

// Signed distance function in object space: negative inside.
using Sdf = std::function<float(glm::vec3)>;

// Samples an SDF at voxel centres over [boundsMin, boundsMax] with `resolution`
// voxels along the longest side. Normals come from the SDF gradient.
// With shellOnly, interior voxels that cannot be seen from outside are dropped,
// which typically saves 80-95% of the leaves of a solid shape.
struct SdfVoxelization {
    VoxelGrid grid;
    glm::vec3 origin;  // object-space position of voxel (0,0,0)'s corner
    float voxelSize;   // object-space edge length of one voxel
};
SdfVoxelization voxelizeSdf(const Sdf& sdf, glm::vec3 boundsMin, glm::vec3 boundsMax, int resolution,
                            const std::function<glm::vec4(glm::vec3)>& color, bool shellOnly = true);

// A raw 8-bit scalar volume (x fastest), e.g. the thesis data sets.
struct RawVolume {
    glm::ivec3 size{0};
    std::vector<uint8_t> data;
    uint8_t at(int x, int y, int z) const { return data[(size_t(z) * size.y + y) * size.x + x]; }
};
// Fails (returns false, fills error) if the file size does not match the dimensions,
// which the original renderer silently ignored.
bool loadRaw(const std::string& path, glm::ivec3 size, RawVolume& out, std::string& error);

// Isosurface voxelisation: voxels with density >= threshold become solid.
// Normals are the negated, signed central-difference gradient (pointing out of
// the dense region). Colour comes from `ramp`, indexed by normalised density.
VoxelGrid voxelizeVolume(const RawVolume& volume, uint8_t threshold,
                         const std::function<glm::vec4(float)>& ramp, bool shellOnly = true);

// Removes occupied voxels whose six neighbours are all occupied.
void keepShell(VoxelGrid& grid);

}  // namespace svo
