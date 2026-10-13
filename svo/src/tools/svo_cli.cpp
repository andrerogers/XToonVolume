// Headless renderer: builds the demo scene and writes PNGs using the CPU
// reference path. Useful on machines without a GPU and for regression images.

#include <chrono>
#include <cstdio>

#include "options.h"

using namespace svo;

int main(int argc, char** argv)
{
    CommonOptions o;
    std::string out = "frame.png";
    int frames = 1;
    float fps = 12.0f;
    glm::vec3 eye(0.0f, 3.2f, 11.0f), target(0.0f, 1.0f, 0.0f);
    for (int i = 1; i < argc; ++i) {
        if (parseCommon(argc, argv, i, o))
            continue;
        if (!std::strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
        else if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) frames = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--fps") && i + 1 < argc) fps = float(std::atof(argv[++i]));
        else if (!std::strcmp(argv[i], "--eye") && i + 1 < argc) std::sscanf(argv[++i], "%f,%f,%f", &eye.x, &eye.y, &eye.z);
        else if (!std::strcmp(argv[i], "--target") && i + 1 < argc) std::sscanf(argv[++i], "%f,%f,%f", &target.x, &target.y, &target.z);
        else {
            std::printf("usage: svo_cli [options]\n%s"
                        "  --out FILE.png           output (with --frames > 1, _NNN is inserted before .png)\n"
                        "  --frames N --fps F       render an animation sequence\n"
                        "  --eye X,Y,Z --target X,Y,Z\n",
                        commonHelp());
            return std::strcmp(argv[i], "--help") ? 2 : 0;
        }
    }

    using Clock = std::chrono::steady_clock;
    std::string error;
    Image toon;
    if (!loadToon(o.toon, toon, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }

    auto t0 = Clock::now();
    Demo demo;
    if (!demo.build(o.demo, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    const std::vector<Node> nodes = demo.scene.packNodes();
    std::printf("built %zu models, %zu nodes (%.1f MB) in %.2fs\n", demo.scene.models.size(), nodes.size(),
                double(nodes.size() * sizeof(Node)) / 1e6,
                std::chrono::duration<double>(Clock::now() - t0).count());

    const Camera camera = Camera::lookAt(eye, target, 45.0f, float(o.width) / float(o.height));
    for (int f = 0; f < frames; ++f) {
        demo.update(o.time + float(f) / fps);
        const FrameData frame = demo.scene.prepareFrame();
        t0 = Clock::now();
        const Image img = renderCpu(frame, nodes, camera, o.width, o.height, o.render, toon);
        std::string path = out;
        if (frames > 1) {
            char suffix[16];
            std::snprintf(suffix, sizeof(suffix), "_%03d", f);
            const size_t dot = path.rfind(".png");
            path.insert(dot == std::string::npos ? path.size() : dot, suffix);
        }
        if (!writePng(path, img)) {
            std::fprintf(stderr, "cannot write %s\n", path.c_str());
            return 1;
        }
        std::printf("%s: %zu instances, %.0f ms\n", path.c_str(), frame.instances.size(),
                    std::chrono::duration<double, std::milli>(Clock::now() - t0).count());
    }
    return 0;
}
