#include "demo.h"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "voxelize.h"

namespace svo {

namespace {

float sdSphere(glm::vec3 p, glm::vec3 c, float r) { return glm::length(p - c) - r; }

float sdCapsule(glm::vec3 p, glm::vec3 a, glm::vec3 b, float r)
{
    const glm::vec3 pa = p - a, ba = b - a;
    const float h = glm::clamp(glm::dot(pa, ba) / glm::dot(ba, ba), 0.0f, 1.0f);
    return glm::length(pa - ba * h) - r;
}

float sdRoundBox(glm::vec3 p, glm::vec3 c, glm::vec3 half, float r)
{
    const glm::vec3 q = glm::abs(p - c) - half + r;
    return glm::length(glm::max(q, 0.0f)) + std::min(std::max({q.x, q.y, q.z}), 0.0f) - r;
}

float smoothUnion(float a, float b, float k)
{
    const float h = glm::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return glm::mix(b, a, h) - k * h * (1.0f - h);
}

// Cheap hash noise for colour variation.
float hash(glm::vec3 p)
{
    const float h = glm::dot(glm::floor(p), glm::vec3(127.1f, 311.7f, 74.7f));
    return glm::fract(std::sin(h) * 43758.5453f);
}

Model makeModel(const std::string& name, const Sdf& sdf, glm::vec3 lo, glm::vec3 hi, int resolution,
                const std::function<glm::vec4(glm::vec3)>& color)
{
    SdfVoxelization v = voxelizeSdf(sdf, lo, hi, resolution, color);
    Model m;
    m.name = name;
    m.tree = svo::build(v.grid);
    m.voxelToObject = glm::translate(glm::mat4(1.0f), v.origin) * glm::scale(glm::mat4(1.0f), glm::vec3(v.voxelSize));
    return m;
}

const glm::vec4 kShirt(0.92f, 0.92f, 0.9f, 1.0f);  // white, so per-walker tint shows through
const glm::vec4 kPants(0.25f, 0.3f, 0.48f, 1.0f);
const glm::vec4 kSkin(0.96f, 0.78f, 0.62f, 1.0f);
const glm::vec4 kShoe(0.18f, 0.15f, 0.14f, 1.0f);

}  // namespace

bool Demo::build(const DemoOptions& o, std::string& error)
{
    scene = Scene();
    parts_.clear();
    walkers_.clear();
    skeleton_ = Skeleton();

    // Ground: a thin slab with a 1 m checkerboard.
    groundModel_ = scene.addModel(makeModel(
        "ground", [](glm::vec3 p) { return sdRoundBox(p, glm::vec3(0, -0.1f, 0), glm::vec3(10, 0.1f, 10), 0.0f); },
        glm::vec3(-10, -0.2f, -10), glm::vec3(10, 0.0f, 10), o.groundResolution, [](glm::vec3 p) {
            const bool odd = (int(std::floor(p.x)) + int(std::floor(p.z))) & 1;
            return odd ? glm::vec4(0.78f, 0.82f, 0.62f, 1) : glm::vec4(0.66f, 0.74f, 0.5f, 1);
        }));

    // Centrepiece: the thesis volume if given, otherwise a procedural tree.
    if (!o.volumePath.empty()) {
        RawVolume raw;
        if (!loadRaw(o.volumePath, o.volumeSize, raw, error))
            return false;
        Model m;
        m.name = "volume";
        m.tree = svo::build(voxelizeVolume(raw, uint8_t(o.volumeThreshold), [](float d) {
            return glm::mix(glm::vec4(0.42f, 0.66f, 0.3f, 1), glm::vec4(0.58f, 0.4f, 0.26f, 1),
                            glm::smoothstep(0.25f, 0.55f, d));
        }));
        // Fit the largest dimension to 3 m, centred on the origin, resting on the ground.
        const float s = 3.0f / float(std::max({o.volumeSize.x, o.volumeSize.y, o.volumeSize.z}));
        m.voxelToObject = glm::scale(glm::mat4(1.0f), glm::vec3(s)) *
                          glm::translate(glm::mat4(1.0f), glm::vec3(-o.volumeSize.x * 0.5f, 0.0f, -o.volumeSize.z * 0.5f));
        centreModel_ = scene.addModel(std::move(m));
    } else {
        centreModel_ = scene.addModel(makeModel(
            "tree",
            [](glm::vec3 p) {
                float d = sdCapsule(p, glm::vec3(0, 0, 0), glm::vec3(0.05f, 1.3f, 0), 0.17f);
                d = smoothUnion(d, sdCapsule(p, glm::vec3(0.02f, 0.9f, 0), glm::vec3(0.55f, 1.45f, 0.15f), 0.08f), 0.1f);
                d = smoothUnion(d, sdCapsule(p, glm::vec3(0.03f, 1.0f, 0), glm::vec3(-0.5f, 1.55f, -0.25f), 0.08f), 0.1f);
                float c = sdSphere(p, glm::vec3(0, 1.85f, 0), 0.75f);
                c = smoothUnion(c, sdSphere(p, glm::vec3(0.62f, 1.55f, 0.2f), 0.5f), 0.25f);
                c = smoothUnion(c, sdSphere(p, glm::vec3(-0.55f, 1.65f, -0.3f), 0.55f), 0.25f);
                c = smoothUnion(c, sdSphere(p, glm::vec3(0.1f, 2.35f, 0.1f), 0.45f), 0.25f);
                return std::min(d, c);
            },
            glm::vec3(-1.3f, 0.0f, -1.3f), glm::vec3(1.3f, 2.9f, 1.3f), 192, [](glm::vec3 p) {
                const bool trunk = p.y < 1.25f && glm::length(glm::vec2(p.x, p.z)) < 0.3f;
                if (trunk)
                    return glm::vec4(0.5f, 0.34f, 0.22f, 1);
                const float n = hash(p * 6.0f);
                return glm::vec4(0.35f + 0.1f * n, 0.62f + 0.12f * n, 0.3f, 1);
            }));
    }

    // Skeleton: joints hang limbs along -y; the walker faces +z.
    const int hips = skeleton_.addJoint("hips", -1, {0, 0.98f, 0});
    const int spine = skeleton_.addJoint("spine", hips, {0, 0.05f, 0});
    const int neck = skeleton_.addJoint("neck", spine, {0, 0.56f, 0});
    const int shoulderL = skeleton_.addJoint("shoulder.L", spine, {0.25f, 0.48f, 0});
    const int elbowL = skeleton_.addJoint("elbow.L", shoulderL, {0, -0.3f, 0});
    const int shoulderR = skeleton_.addJoint("shoulder.R", spine, {-0.25f, 0.48f, 0});
    const int elbowR = skeleton_.addJoint("elbow.R", shoulderR, {0, -0.3f, 0});
    const int hipL = skeleton_.addJoint("hip.L", hips, {0.11f, -0.02f, 0});
    const int kneeL = skeleton_.addJoint("knee.L", hipL, {0, -0.45f, 0});
    const int hipR = skeleton_.addJoint("hip.R", hips, {-0.11f, -0.02f, 0});
    const int kneeR = skeleton_.addJoint("knee.R", hipR, {0, -0.45f, 0});

    // One-second walk cycle. Positive X rotation swings a hanging limb backwards.
    skeleton_.duration = 1.0f;
    const float k[4] = {0.0f, 0.25f, 0.5f, 0.75f};
    const float thigh[4] = {-28, 0, 28, 0}, knee[4] = {5, 10, 15, 55};
    const float shoulder[4] = {24, 0, -24, 0}, elbow[4] = {-10, -20, -35, -20};
    for (int i = 0; i < 4; ++i) {
        const int j = (i + 2) % 4;  // the right side runs half a cycle behind
        skeleton_.addKey(hips, k[i], {0, i == 0 ? 6.0f : i == 2 ? -6.0f : 0.0f, 0}, {0, (i & 1) ? 0.04f : 0.0f, 0});
        skeleton_.addKey(spine, k[i], {4, i == 0 ? -9.0f : i == 2 ? 9.0f : 0.0f, 0});
        skeleton_.addKey(hipL, k[i], {thigh[i], 0, 0});
        skeleton_.addKey(hipR, k[i], {thigh[j], 0, 0});
        skeleton_.addKey(kneeL, k[i], {knee[i], 0, 0});
        skeleton_.addKey(kneeR, k[i], {knee[j], 0, 0});
        skeleton_.addKey(shoulderL, k[i], {shoulder[i], 0, 4});
        skeleton_.addKey(shoulderR, k[i], {shoulder[j], 0, -4});
        skeleton_.addKey(elbowL, k[i], {elbow[i], 0, 0});
        skeleton_.addKey(elbowR, k[i], {elbow[j], 0, 0});
    }

    // Limb octrees, in joint-local space. Left and right limbs share one model.
    const int res = o.partResolution;
    const uint32_t torso = scene.addModel(makeModel(
        "torso",
        [](glm::vec3 p) {
            return smoothUnion(sdRoundBox(p, {0, 0.28f, 0}, {0.18f, 0.25f, 0.11f}, 0.07f),
                               sdRoundBox(p, {0, 0.0f, 0}, {0.16f, 0.09f, 0.1f}, 0.05f), 0.05f);
        },
        {-0.22f, -0.11f, -0.14f}, {0.22f, 0.56f, 0.14f}, res,
        [](glm::vec3 p) { return p.y < 0.07f ? kPants : kShirt; }));
    const uint32_t head = scene.addModel(makeModel(
        "head",
        [](glm::vec3 p) {
            float d = smoothUnion(sdSphere(p, {0, 0.16f, 0}, 0.14f), sdCapsule(p, {0, -0.04f, 0}, {0, 0.06f, 0}, 0.055f), 0.03f);
            return std::min(d, sdSphere(p, {0, 0.14f, 0.14f}, 0.028f));
        },
        {-0.16f, -0.1f, -0.16f}, {0.16f, 0.31f, 0.18f}, res, [](glm::vec3 p) {
            const bool eye = p.z > 0.09f && std::abs(std::abs(p.x) - 0.055f) < 0.025f && std::abs(p.y - 0.19f) < 0.025f;
            if (eye)
                return glm::vec4(0.1f, 0.1f, 0.12f, 1);
            if (p.y > 0.2f && p.z < 0.1f)
                return glm::vec4(0.35f, 0.22f, 0.15f, 1);  // hair
            return kSkin;
        }));
    const uint32_t upperArm = scene.addModel(makeModel(
        "upper_arm", [](glm::vec3 p) { return sdCapsule(p, {0, 0, 0}, {0, -0.3f, 0}, 0.065f); },
        {-0.08f, -0.38f, -0.08f}, {0.08f, 0.08f, 0.08f}, res, [](glm::vec3) { return kShirt; }));
    const uint32_t forearm = scene.addModel(makeModel(
        "forearm",
        [](glm::vec3 p) {
            return smoothUnion(sdCapsule(p, {0, 0, 0}, {0, -0.24f, 0}, 0.055f), sdSphere(p, {0, -0.3f, 0.01f}, 0.065f), 0.03f);
        },
        {-0.08f, -0.38f, -0.08f}, {0.08f, 0.07f, 0.09f}, res,
        [](glm::vec3 p) { return p.y < -0.22f ? kSkin : kShirt; }));
    const uint32_t thighModel = scene.addModel(makeModel(
        "thigh", [](glm::vec3 p) { return sdCapsule(p, {0, 0, 0}, {0, -0.45f, 0}, 0.085f); },
        {-0.1f, -0.55f, -0.1f}, {0.1f, 0.1f, 0.1f}, res, [](glm::vec3) { return kPants; }));
    const uint32_t shin = scene.addModel(makeModel(
        "shin",
        [](glm::vec3 p) {
            return smoothUnion(sdCapsule(p, {0, 0, 0}, {0, -0.42f, 0}, 0.07f),
                               sdRoundBox(p, {0, -0.47f, 0.05f}, {0.065f, 0.04f, 0.12f}, 0.025f), 0.03f);
        },
        {-0.1f, -0.53f, -0.1f}, {0.1f, 0.08f, 0.2f}, res,
        [](glm::vec3 p) { return p.y < -0.4f ? kShoe : kPants; }));

    parts_ = {{torso, spine},       {head, neck},          {upperArm, shoulderL}, {upperArm, shoulderR},
              {forearm, elbowL},    {forearm, elbowR},     {thighModel, hipL},    {thighModel, hipR},
              {shin, kneeL},        {shin, kneeR}};

    const glm::vec4 palette[6] = {{0.95f, 0.45f, 0.4f, 1}, {0.4f, 0.65f, 0.95f, 1}, {0.98f, 0.8f, 0.35f, 1},
                                  {0.55f, 0.85f, 0.5f, 1}, {0.8f, 0.55f, 0.9f, 1},  {1.0f, 1.0f, 1.0f, 1}};
    for (int i = 0; i < o.walkers; ++i) {
        Walker w;
        w.radius = (i & 1) ? 5.5f : 3.6f;
        w.angle0 = 6.2831853f * float(i) / float(std::max(o.walkers, 1));
        w.phase = hash(glm::vec3(float(i), 3.0f, 7.0f));
        w.tint = palette[i % 6];
        walkers_.push_back(w);
    }
    update(0.0f);
    return true;
}

void Demo::update(float time)
{
    scene.instances.clear();
    scene.instances.push_back({groundModel_, glm::mat4(1.0f), glm::vec4(1.0f)});
    scene.instances.push_back(
        {centreModel_, glm::rotate(glm::mat4(1.0f), time * centrepieceSpin, glm::vec3(0, 1, 0)), glm::vec4(1.0f)});

    const float walkSpeed = 1.1f;  // m/s, roughly matches the stride of the 1 s cycle
    for (const Walker& w : walkers_) {
        const float t = time * speed;
        skeleton_.pose(t + w.phase, pose_);
        const float a = w.angle0 + t * walkSpeed / w.radius;
        const glm::mat4 root = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(w.radius * std::cos(a), 0.0f,
                                                                                         w.radius * std::sin(a))),
                                           -a, glm::vec3(0, 1, 0));
        for (const Part& p : parts_) {
            const glm::vec4 tint = scene.models[p.model].name == "head" ? glm::vec4(1.0f) : w.tint;
            scene.instances.push_back({p.model, root * pose_[p.joint], tint});
        }
    }
}

}  // namespace svo
