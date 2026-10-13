#include "scene.h"

namespace svo {

uint32_t Scene::addModel(Model m)
{
    models.push_back(std::move(m));
    return uint32_t(models.size() - 1);
}

size_t Scene::totalNodes() const
{
    size_t n = 0;
    for (const auto& m : models)
        n += m.tree.nodes.size();
    return n;
}

std::vector<Node> Scene::packNodes()
{
    std::vector<Node> packed;
    packed.reserve(totalNodes());
    for (auto& m : models) {
        m.nodeOffset = uint32_t(packed.size());
        for (Node n : m.tree.nodes) {
            if (n.childMask)
                n.childBase += m.nodeOffset;
            packed.push_back(n);
        }
    }
    return packed;
}

FrameData Scene::prepareFrame() const
{
    std::vector<Aabb> boxes;
    std::vector<GpuInstance> unordered;
    boxes.reserve(instances.size());
    unordered.reserve(instances.size());
    for (const auto& inst : instances) {
        const Model& m = models[inst.model];
        if (m.tree.empty())
            continue;
        const glm::mat4 voxelToWorld = inst.objectToWorld * m.voxelToObject;
        boxes.push_back(transformAabb(voxelToWorld, glm::vec3(0.0f), glm::vec3(float(m.tree.resolution()))));
        GpuInstance g;
        g.voxelToWorld = voxelToWorld;
        g.worldToVoxel = glm::inverse(voxelToWorld);
        g.rootNode = m.nodeOffset;
        g.depth = uint32_t(m.tree.depth);
        g.voxelSize = glm::length(glm::vec3(voxelToWorld[0]));
        g.tint = packColor(inst.tint);
        unordered.push_back(g);
    }

    FrameData frame;
    const Bvh bvh = buildBvh(boxes);
    frame.bvh = bvh.nodes;
    frame.instances.reserve(unordered.size());
    for (uint32_t i : bvh.order)
        frame.instances.push_back(unordered[i]);
    return frame;
}

}  // namespace svo
