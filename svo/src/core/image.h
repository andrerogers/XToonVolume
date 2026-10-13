#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace svo {

// RGBA8 image. Row 0 is the bottom row, matching OpenGL texture and
// glReadPixels conventions; writePng flips it so files are upright.
struct Image {
    int width = 0, height = 0;
    std::vector<uint8_t> rgba;

    Image() = default;
    Image(int w, int h) : width(w), height(h), rgba(size_t(w) * h * 4, 0) {}
    uint8_t* at(int x, int y) { return &rgba[(size_t(y) * width + x) * 4]; }
    const uint8_t* at(int x, int y) const { return &rgba[(size_t(y) * width + x) * 4]; }
    void set(int x, int y, glm::vec4 c);
    glm::vec4 get(int x, int y) const;
    // Bilinear, clamp-to-edge, texel centres at (i + 0.5) / size like GL_LINEAR.
    glm::vec4 sample(glm::vec2 uv) const;
};

// Uncompressed 24/32-bit TGA (what the thesis transfer textures are).
bool loadTga(const std::string& path, Image& out, std::string& error);
// Uncompressed (stored deflate) PNG: no zlib dependency, larger files.
bool writePng(const std::string& path, const Image& img);

// Procedural X-Toon textures. Alpha is the weight of the voxel's own colour:
// 1 = tint the toon tone by the voxel colour, 0 = use the toon colour as is
// (rims and highlights).
enum class ToonPreset { Bands, Backlight, Highlight };
Image makeToonTexture(ToonPreset preset, int size = 256);

}  // namespace svo
