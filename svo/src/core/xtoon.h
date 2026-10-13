#pragma once

// X-Toon detail functions (Barla, Thollot & Markosian, NPAR 2006).
//
// The toon texture is indexed by (tone, detail): tone = max(n.l, 0) along u,
// and detail D in [0, 1] along v. D = 1 is full detail, D = 0 fully abstracted.
// Every function clamps to [0, 1] and is defined for every input, which fixes
// the NaN/Inf cases in the original rayCastXToon.frag (r = 1, pow of a negative base).
//
// shaders/svo.glsl has an identical copy of these.

#include <algorithm>
#include <cmath>

namespace svo::xtoon {

enum class DetailMode : int {
    Off = 0,          // D = 1 everywhere: plain toon shading
    Depth = 1,        // LOD: D = 1 - log(z / zMin) / log(r), with zMax = r * zMin
    Focus = 2,        // depth of field around a focal distance
    Backlight = 3,    // D = |n.v|^r: abstraction towards silhouettes
    Highlight = 4,    // D = max(v.reflect(l), 0)^s: highlights
    OctreeLevel = 5,  // new: D = level / depth, coupling abstraction to the SVO's LOD
};

inline float clamp01(float x) { return std::min(std::max(x, 0.0f), 1.0f); }

inline float detailDepth(float z, float zMin, float r)
{
    r = std::max(r, 1.001f);  // r = 1 would make zMax == zMin and divide by log(1) = 0
    zMin = std::max(zMin, 1e-4f);
    if (z <= zMin)
        return 1.0f;
    return clamp01(1.0f - std::log(z / zMin) / std::log(r));
}

// Symmetric variant: full detail within +-width of the focal distance, falling
// off logarithmically to zero at r * width away from it.
inline float detailFocus(float z, float focus, float width, float r)
{
    return detailDepth(std::abs(z - focus), width, r);
}

inline float detailBacklight(float nDotV, float r) { return std::pow(std::abs(nDotV), std::max(r, 0.0f)); }

inline float detailHighlight(float vDotR, float s) { return std::pow(clamp01(vDotR), std::max(s, 0.0f)); }

inline float detailLevel(int level, int depth) { return depth > 0 ? clamp01(float(level) / float(depth)) : 1.0f; }

}  // namespace svo::xtoon
