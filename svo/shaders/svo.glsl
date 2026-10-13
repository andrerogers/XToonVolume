// Shared GLSL: data layouts, octree traversal, scene traversal and X-Toon.
// This is a line-by-line port of src/core/{traverse,render}.cpp and xtoon.h;
// keep them in sync (the viewer's --compare mode checks they agree).

// --- Buffers (layouts match the static_asserted C++ structs) ---

// Node: x = childBase, y = childMask (0 => leaf), z = RGBA8 colour, w = octahedral normal.
layout(std430, binding = 0) readonly buffer Nodes { uvec4 nodes[]; };

struct Instance {
    mat4 worldToVoxel;
    mat4 voxelToWorld;
    uint rootNode;
    uint depth;
    float voxelSize;
    uint tint;
};
layout(std430, binding = 1) readonly buffer Instances { Instance instances[]; };

struct BvhNode {
    vec3 lo;
    uint first;
    vec3 hi;
    uint count;  // > 0 => leaf over instances[first .. first + count)
};
layout(std430, binding = 2) readonly buffer Bvh { BvhNode bvh[]; };

uniform int uInstanceCount;
uniform float uLodBias;
uniform int uMaxSteps;

// --- Packing ---

vec4 unpackColor(uint c) { return unpackUnorm4x8(c); }

vec3 unpackNormal(uint p)
{
    vec2 e = max(unpackSnorm2x16(p), vec2(-1.0));
    vec3 n = vec3(e, 1.0 - abs(e.x) - abs(e.y));
    if (n.z < 0.0)
        n.xy = (1.0 - abs(n.yx)) * vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);
    return normalize(n);
}

// --- Octree traversal (see traverse.cpp for the explanation) ---

struct Hit {
    bool hit;
    float t;
    int level;
    uint node;
    int axis;
    int steps;
};

vec3 safeDir(vec3 d)
{
    for (int a = 0; a < 3; ++a)
        if (abs(d[a]) < 1e-12)
            d[a] = d[a] < 0.0 ? -1e-12 : 1e-12;
    return d;
}

int argMin3(vec3 v) { return v.x <= v.y ? (v.x <= v.z ? 0 : 2) : (v.y <= v.z ? 1 : 2); }
int argMax3(vec3 v) { return v.x >= v.y ? (v.x >= v.z ? 0 : 2) : (v.y >= v.z ? 1 : 2); }

Hit traverseOctree(uint root, int depth, vec3 o, vec3 dir, float tMin, float tMax, float coneK, int maxSteps)
{
    Hit hit = Hit(false, 0.0, 0, 0u, 0, 0);
    int res = 1 << depth;
    vec3 d = safeDir(dir);
    vec3 invD = 1.0 / d;

    vec3 t0 = -o * invD;
    vec3 t1 = (vec3(float(res)) - o) * invD;
    vec3 tNear = min(t0, t1), tFar = max(t0, t1);
    float t = max(max(tNear.x, tNear.y), max(tNear.z, tMin));
    float tExit = min(min(tFar.x, tFar.y), min(tFar.z, tMax));
    if (t > tExit)
        return hit;

    hit.axis = argMax3(tNear);
    ivec3 cell = clamp(ivec3(floor(o + d * t)), ivec3(0), ivec3(res - 1));

    uint stack[17];
    stack[0] = root;
    int level = 0;

    for (int step = 0; step < maxSteps; ++step) {
        int emptyLevel;
        for (;;) {
            uvec4 n = nodes[stack[level]];
            int size = res >> level;
            if (n.y == 0u || (coneK > 0.0 && float(size) <= coneK * t)) {
                hit.hit = true;
                hit.t = t;
                hit.level = level;
                hit.node = stack[level];
                hit.steps = step;
                return hit;
            }
            int shift = depth - level - 1;
            uint oct = uint((cell.x >> shift) & 1) | (uint((cell.y >> shift) & 1) << 1) | (uint((cell.z >> shift) & 1) << 2);
            if ((n.y & (1u << oct)) == 0u) {
                emptyLevel = level + 1;
                break;
            }
            stack[level + 1] = n.x + uint(bitCount(n.y & ((1u << oct) - 1u)));
            ++level;
        }

        int shift = depth - emptyLevel;
        int size = 1 << shift;
        ivec3 cellMin = (cell >> shift) << shift;
        vec3 bound = vec3(cellMin) + vec3(d.x > 0.0 ? size : 0, d.y > 0.0 ? size : 0, d.z > 0.0 ? size : 0);
        vec3 tm = (bound - o) * invD;
        int axis = argMin3(tm);
        t = max(t, tm[axis]);
        if (t > tExit)
            return hit;

        ivec3 next = clamp(ivec3(floor(o + d * t)), cellMin, cellMin + size - 1);
        next[axis] = d[axis] > 0.0 ? cellMin[axis] + size : cellMin[axis] - 1;
        if (next[axis] < 0 || next[axis] >= res)
            return hit;

        int diff = (cell.x ^ next.x) | (cell.y ^ next.y) | (cell.z ^ next.z);
        level = min(level, depth - 1 - findMSB(diff));
        cell = next;
        hit.axis = axis;
    }
    return hit;
}

// --- Scene traversal through the top-level BVH ---

struct SceneHit {
    bool hit;
    float t;
    uint instance;
    Hit local;
    int steps;
};

float intersectAabb(vec3 lo, vec3 hi, vec3 o, vec3 invD, float tMax)
{
    vec3 t0 = (lo - o) * invD, t1 = (hi - o) * invD;
    vec3 tn = min(t0, t1), tf = max(t0, t1);
    float enter = max(max(tn.x, tn.y), max(tn.z, 0.0));
    float exit = min(min(tf.x, tf.y), min(tf.z, tMax));
    return enter <= exit ? enter : -1.0;
}

SceneHit traceScene(vec3 origin, vec3 dir, float tMax, float pixelSpread, bool anyHit)
{
    SceneHit best = SceneHit(false, 0.0, 0u, Hit(false, 0.0, 0, 0u, 0, 0), 0);
    if (uInstanceCount == 0)
        return best;
    vec3 safe = dir;
    for (int a = 0; a < 3; ++a)
        if (abs(safe[a]) < 1e-12)
            safe[a] = 1e-12;
    vec3 invD = 1.0 / safe;

    uint stack[64];
    int sp = 0;
    stack[sp++] = 0u;
    float closest = tMax;
    while (sp > 0) {
        BvhNode node = bvh[stack[--sp]];
        if (intersectAabb(node.lo, node.hi, origin, invD, closest) < 0.0)
            continue;
        if (node.count == 0u) {
            BvhNode a = bvh[node.first];
            BvhNode b = bvh[node.first + 1u];
            float ta = intersectAabb(a.lo, a.hi, origin, invD, closest);
            float tb = intersectAabb(b.lo, b.hi, origin, invD, closest);
            bool aFirst = tb < 0.0 || (ta >= 0.0 && ta <= tb);
            stack[sp++] = aFirst ? node.first + 1u : node.first;
            stack[sp++] = aFirst ? node.first : node.first + 1u;
            continue;
        }
        for (uint i = node.first; i < node.first + node.count; ++i) {
            vec3 lo = (instances[i].worldToVoxel * vec4(origin, 1.0)).xyz;
            vec3 ld = mat3(instances[i].worldToVoxel) * dir;
            float coneK = uLodBias > 0.0 ? pixelSpread * uLodBias / instances[i].voxelSize : 0.0;
            Hit h = traverseOctree(instances[i].rootNode, int(instances[i].depth), lo, ld, 0.0, closest, coneK, uMaxSteps);
            best.steps += h.steps;
            if (h.hit && h.t < closest) {
                closest = h.t;
                best.hit = true;
                best.t = h.t;
                best.instance = i;
                best.local = h;
                if (anyHit)
                    return best;
            }
        }
    }
    return best;
}

// --- X-Toon detail functions (see xtoon.h) ---

float detailDepth(float z, float zMin, float r)
{
    r = max(r, 1.001);
    zMin = max(zMin, 1e-4);
    if (z <= zMin)
        return 1.0;
    return clamp(1.0 - log(z / zMin) / log(r), 0.0, 1.0);
}

float detailFocus(float z, float focus, float width, float r) { return detailDepth(abs(z - focus), width, r); }

float detailBacklight(float nDotV, float r) { return pow(abs(nDotV), max(r, 0.0)); }

float detailHighlight(float vDotR, float s) { return pow(clamp(vDotR, 0.0, 1.0), max(s, 0.0)); }

float detailLevel(int level, int depth) { return depth > 0 ? clamp(float(level) / float(depth), 0.0, 1.0) : 1.0; }
