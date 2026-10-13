#pragma once

// Command-line options shared by svo_cli and the viewer.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "demo.h"
#include "image.h"
#include "render.h"

namespace svo {

struct CommonOptions {
    DemoOptions demo;
    RenderSettings render;
    std::string toon = "bands";  // bands | backlight | highlight | path to a .tga
    int width = 960, height = 540;
    float time = 0.0f;
};

// "path:WxHxD[:threshold]", e.g. ../VolRender/bonsai.raw:256x256x256:40
inline bool parseVolume(const char* arg, DemoOptions& demo)
{
    std::string s = arg;
    const size_t c1 = s.find(':');
    if (c1 == std::string::npos)
        return false;
    demo.volumePath = s.substr(0, c1);
    int x, y, z, thr = demo.volumeThreshold;
    const int n = std::sscanf(s.c_str() + c1 + 1, "%dx%dx%d:%d", &x, &y, &z, &thr);
    if (n < 3)
        return false;
    demo.volumeSize = glm::ivec3(x, y, z);
    demo.volumeThreshold = thr;
    return true;
}

// Consumes one option at argv[i] (and its value). Returns false if unknown.
inline bool parseCommon(int argc, char** argv, int& i, CommonOptions& o)
{
    auto next = [&]() -> const char* {
        if (i + 1 >= argc) {
            std::fprintf(stderr, "missing value for %s\n", argv[i]);
            std::exit(2);
        }
        return argv[++i];
    };
    const char* a = argv[i];
    if (!std::strcmp(a, "--volume")) {
        if (!parseVolume(next(), o.demo)) {
            std::fprintf(stderr, "--volume expects path:WxHxD[:threshold]\n");
            std::exit(2);
        }
    } else if (!std::strcmp(a, "--walkers")) o.demo.walkers = std::atoi(next());
    else if (!std::strcmp(a, "--part-res")) o.demo.partResolution = std::atoi(next());
    else if (!std::strcmp(a, "--size")) {
        if (std::sscanf(next(), "%dx%d", &o.width, &o.height) != 2) {
            std::fprintf(stderr, "--size expects WxH\n");
            std::exit(2);
        }
    } else if (!std::strcmp(a, "--time")) o.time = float(std::atof(next()));
    else if (!std::strcmp(a, "--toon")) o.toon = next();
    else if (!std::strcmp(a, "--detail")) o.render.detail = xtoon::DetailMode(std::atoi(next()));
    else if (!std::strcmp(a, "--view")) o.render.view = View(std::atoi(next()));
    else if (!std::strcmp(a, "--lod")) o.render.lodBias = float(std::atof(next()));
    else if (!std::strcmp(a, "--no-shadows")) o.render.shadows = false;
    else if (!std::strcmp(a, "--no-outlines")) o.render.outlines = false;
    else if (!std::strcmp(a, "--blocky")) o.render.smoothNormals = false;
    else return false;
    return true;
}

inline const char* commonHelp()
{
    return "  --volume PATH:WxHxD[:T]  use a raw 8-bit volume as the centrepiece (iso threshold T, default 40)\n"
           "  --walkers N              crowd size (default 12)\n"
           "  --part-res N             voxels along each limb's longest side (default 64)\n"
           "  --size WxH               image size (default 960x540)\n"
           "  --time SECONDS           animation time\n"
           "  --toon NAME|FILE.tga     bands | backlight | highlight | a TGA toon texture\n"
           "  --detail N               0 off, 1 depth, 2 focus, 3 backlight, 4 highlight, 5 octree level\n"
           "  --view N                 0 x-toon, 1 albedo, 2 normals, 3 octree level, 4 cost, 5 detail\n"
           "  --lod BIAS               LOD footprint in pixels, 0 = always full resolution (default 1)\n"
           "  --no-shadows --no-outlines --blocky\n";
}

inline bool loadToon(const std::string& name, Image& out, std::string& error)
{
    if (name == "bands") out = makeToonTexture(ToonPreset::Bands);
    else if (name == "backlight") out = makeToonTexture(ToonPreset::Backlight);
    else if (name == "highlight") out = makeToonTexture(ToonPreset::Highlight);
    else return loadTga(name, out, error);
    return true;
}

}  // namespace svo
