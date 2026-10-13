#include "render.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <thread>

namespace svo {

Camera Camera::lookAt(glm::vec3 position, glm::vec3 target, float fovYDegrees, float aspect)
{
    Camera c;
    c.position = position;
    c.forward = glm::normalize(target - position);
    c.right = glm::normalize(glm::cross(c.forward, glm::vec3(0, 1, 0)));
    c.up = glm::cross(c.right, c.forward);
    c.tanHalfFovY = std::tan(glm::radians(fovYDegrees) * 0.5f);
    c.aspect = aspect;
    return c;
}

glm::vec3 Camera::rayDir(glm::vec2 ndc) const
{
    return glm::normalize(forward + right * (ndc.x * aspect * tanHalfFovY) + up * (ndc.y * tanHalfFovY));
}

SceneHit traceScene(const FrameData& frame, const Node* nodes, glm::vec3 origin, glm::vec3 dir, float tMax,
                    float pixelSpread, const RenderSettings& settings, bool anyHit)
{
    SceneHit best;
    if (frame.bvh.empty())
        return best;
    glm::vec3 safe = dir;
    for (int a = 0; a < 3; ++a)
        if (std::abs(safe[a]) < 1e-12f)
            safe[a] = 1e-12f;
    const glm::vec3 invD = 1.0f / safe;

    uint32_t stack[64];
    int sp = 0;
    stack[sp++] = 0;
    float closest = tMax;
    while (sp > 0) {
        const BvhNode& node = frame.bvh[stack[--sp]];
        if (intersectAabb(node.lo, node.hi, origin, invD, closest) < 0.0f)
            continue;
        if (node.count == 0) {
            // Visit the nearer child first so `closest` shrinks sooner.
            const BvhNode& a = frame.bvh[node.first];
            const BvhNode& b = frame.bvh[node.first + 1];
            const float ta = intersectAabb(a.lo, a.hi, origin, invD, closest);
            const float tb = intersectAabb(b.lo, b.hi, origin, invD, closest);
            const bool aFirst = tb < 0.0f || (ta >= 0.0f && ta <= tb);
            stack[sp++] = aFirst ? node.first + 1 : node.first;
            stack[sp++] = aFirst ? node.first : node.first + 1;
            continue;
        }
        for (uint32_t i = node.first; i < node.first + node.count; ++i) {
            const GpuInstance& inst = frame.instances[i];
            const Ray local{glm::vec3(inst.worldToVoxel * glm::vec4(origin, 1.0f)),
                            glm::mat3(inst.worldToVoxel) * dir};
            const float coneK = settings.lodBias > 0.0f ? pixelSpread * settings.lodBias / inst.voxelSize : 0.0f;
            const Hit h = traverse(nodes, inst.rootNode, int(inst.depth), local, 0.0f, closest, coneK, settings.maxSteps);
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

namespace {

glm::vec3 heat(float x)
{
    x = std::clamp(x, 0.0f, 1.0f);
    const glm::vec3 c0(0.1f, 0.15f, 0.5f), c1(0.1f, 0.7f, 0.4f), c2(0.95f, 0.85f, 0.2f), c3(0.85f, 0.15f, 0.1f);
    if (x < 1.0f / 3.0f)
        return glm::mix(c0, c1, x * 3.0f);
    if (x < 2.0f / 3.0f)
        return glm::mix(c1, c2, x * 3.0f - 1.0f);
    return glm::mix(c2, c3, x * 3.0f - 2.0f);
}

glm::vec3 sky(glm::vec3 dir)
{
    return glm::mix(glm::vec3(0.93f, 0.91f, 0.86f), glm::vec3(0.55f, 0.68f, 0.86f), std::clamp(dir.y * 1.5f, 0.0f, 1.0f));
}

struct PixelOut {
    glm::vec3 color;
    float depth;  // ray distance, or -1 for background
    uint32_t id;  // instance + 1, or 0 for background
};

PixelOut shade(const FrameData& frame, const std::vector<Node>& nodes, glm::vec3 origin, glm::vec3 dir,
               float pixelSpread, const RenderSettings& s, const Image& toon)
{
    const SceneHit hit = traceScene(frame, nodes.data(), origin, dir, 1e30f, pixelSpread, s);
    if (s.view == View::Cost)
        return {heat(float(hit.steps) / 256.0f), hit.hit ? hit.t : -1.0f, hit.hit ? hit.instance + 1 : 0};
    if (!hit.hit)
        return {sky(dir), -1.0f, 0};

    const GpuInstance& inst = frame.instances[hit.instance];
    const Node& node = nodes[hit.local.node];
    const glm::vec4 albedo = unpackColor(node.color) * unpackColor(inst.tint);

    glm::vec3 nLocal;
    if (s.smoothNormals) {
        nLocal = unpackNormal(node.normal);
    } else {
        const glm::vec3 dLocal = glm::mat3(inst.worldToVoxel) * dir;
        nLocal = glm::vec3(0.0f);
        nLocal[hit.local.axis] = dLocal[hit.local.axis] > 0.0f ? -1.0f : 1.0f;
    }
    const glm::vec3 n = glm::normalize(glm::mat3(inst.voxelToWorld) * nLocal);
    const glm::vec3 p = origin + dir * hit.t;
    const glm::vec3 v = -dir;
    const glm::vec3 l = s.lightDir;

    float lit = std::max(glm::dot(n, l), 0.0f);
    if (s.shadows && lit > 0.0f) {
        const glm::vec3 shadowOrigin = p + n * (inst.voxelSize * 2.0f);
        if (traceScene(frame, nodes.data(), shadowOrigin, l, 1e30f, pixelSpread, s, true).hit)
            lit = 0.0f;
    }

    float detail = 1.0f;
    switch (s.detail) {
    case xtoon::DetailMode::Off: break;
    case xtoon::DetailMode::Depth: detail = xtoon::detailDepth(hit.t, s.zMin, s.r); break;
    case xtoon::DetailMode::Focus: detail = xtoon::detailFocus(hit.t, s.focus, s.focusWidth, s.r); break;
    case xtoon::DetailMode::Backlight: detail = xtoon::detailBacklight(glm::dot(n, v), s.backlightR); break;
    case xtoon::DetailMode::Highlight:
        detail = xtoon::detailHighlight(glm::dot(v, glm::reflect(-l, n)), s.highlightS);
        break;
    case xtoon::DetailMode::OctreeLevel: detail = xtoon::detailLevel(hit.local.level, int(inst.depth)); break;
    }

    glm::vec3 color;
    switch (s.view) {
    case View::Albedo: color = glm::vec3(albedo) * (0.4f + 0.6f * lit); break;
    case View::Normal: color = n * 0.5f + 0.5f; break;
    case View::Level: color = heat(float(hit.local.level) / float(std::max(1u, inst.depth))); break;
    case View::Detail: color = glm::vec3(detail); break;
    default: {
        const glm::vec4 t = toon.sample(glm::vec2(lit, detail));
        color = glm::mix(glm::vec3(t), glm::vec3(t) * glm::vec3(albedo), t.a * s.albedoMix);
    }
    }
    return {color, hit.t, hit.instance + 1};
}

}  // namespace

Image renderCpu(const FrameData& frame, const std::vector<Node>& nodes, const Camera& camera, int width, int height,
                const RenderSettings& settings, const Image& toon, int threads)
{
    std::vector<PixelOut> px(size_t(width) * height);
    const float pixelSpread = 2.0f * camera.tanHalfFovY / float(height);
    std::atomic<int> nextRow{0};
    auto worker = [&] {
        for (int y; (y = nextRow++) < height;)
            for (int x = 0; x < width; ++x) {
                const glm::vec2 ndc((x + 0.5f) / width * 2.0f - 1.0f, (y + 0.5f) / height * 2.0f - 1.0f);
                px[size_t(y) * width + x] =
                    shade(frame, nodes, camera.position, camera.rayDir(ndc), pixelSpread, settings, toon);
            }
    };
    if (threads <= 0)
        threads = int(std::max(1u, std::thread::hardware_concurrency()));
    std::vector<std::thread> pool;
    for (int i = 0; i < threads; ++i)
        pool.emplace_back(worker);
    for (auto& t : pool)
        t.join();

    // Outline pass: dark lines where the instance changes or depth jumps.
    Image img(width, height);
    const bool outlines = settings.outlines && settings.view == View::XToon;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const PixelOut& c = px[size_t(y) * width + x];
            glm::vec3 color = c.color;
            if (outlines && c.id != 0) {
                const int nx[4] = {x - 1, x + 1, x, x}, ny[4] = {y, y, y - 1, y + 1};
                for (int k = 0; k < 4; ++k) {
                    if (nx[k] < 0 || ny[k] < 0 || nx[k] >= width || ny[k] >= height)
                        continue;
                    const PixelOut& o = px[size_t(ny[k]) * width + nx[k]];
                    if (o.id != c.id || std::abs(o.depth - c.depth) > settings.outlineDepth * c.depth) {
                        color *= 0.12f;
                        break;
                    }
                }
            }
            img.set(x, y, glm::vec4(color, 1.0f));
        }
    return img;
}

}  // namespace svo
