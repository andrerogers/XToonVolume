#include "image.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>

namespace svo {

void Image::set(int x, int y, glm::vec4 c)
{
    uint8_t* p = at(x, y);
    for (int i = 0; i < 4; ++i)
        p[i] = uint8_t(std::clamp(c[i], 0.0f, 1.0f) * 255.0f + 0.5f);
}

glm::vec4 Image::get(int x, int y) const
{
    const uint8_t* p = at(x, y);
    return glm::vec4(p[0], p[1], p[2], p[3]) / 255.0f;
}

glm::vec4 Image::sample(glm::vec2 uv) const
{
    const glm::vec2 p = uv * glm::vec2(width, height) - 0.5f;
    const glm::vec2 f = p - glm::floor(p);
    // Clamping each tap separately reproduces GL_CLAMP_TO_EDGE.
    const int fx = int(std::floor(p.x)), fy = int(std::floor(p.y));
    const int x0 = std::clamp(fx, 0, width - 1), x1 = std::clamp(fx + 1, 0, width - 1);
    const int y0 = std::clamp(fy, 0, height - 1), y1 = std::clamp(fy + 1, 0, height - 1);
    return glm::mix(glm::mix(get(x0, y0), get(x1, y0), f.x), glm::mix(get(x0, y1), get(x1, y1), f.x), f.y);
}

bool loadTga(const std::string& path, Image& out, std::string& error)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        error = "cannot open " + path;
        return false;
    }
    uint8_t h[18];
    if (!f.read(reinterpret_cast<char*>(h), 18)) {
        error = path + ": truncated header";
        return false;
    }
    const int type = h[2], bpp = h[16];
    const int w = h[12] | (h[13] << 8), hgt = h[14] | (h[15] << 8);
    if (type != 2 || (bpp != 24 && bpp != 32) || w <= 0 || hgt <= 0) {
        error = path + ": only uncompressed 24/32-bit TGA is supported";
        return false;
    }
    f.seekg(18 + h[0]);  // skip the image ID field
    const int channels = bpp / 8;
    std::vector<uint8_t> src(size_t(w) * hgt * channels);
    if (!f.read(reinterpret_cast<char*>(src.data()), std::streamsize(src.size()))) {
        error = path + ": truncated pixel data";
        return false;
    }
    const bool topOrigin = (h[17] & 0x20) != 0;
    out = Image(w, hgt);
    for (int y = 0; y < hgt; ++y)
        for (int x = 0; x < w; ++x) {
            const uint8_t* s = &src[(size_t(y) * w + x) * channels];
            uint8_t* d = out.at(x, topOrigin ? hgt - 1 - y : y);
            d[0] = s[2], d[1] = s[1], d[2] = s[0], d[3] = channels == 4 ? s[3] : 255;
        }
    return true;
}

namespace {

uint32_t crc32(const uint8_t* data, size_t n, uint32_t crc = 0)
{
    static const auto table = [] {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
            t[i] = c;
        }
        return t;
    }();
    crc = ~crc;
    for (size_t i = 0; i < n; ++i)
        crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    return ~crc;
}

void put32(std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back(uint8_t(x >> 24)), v.push_back(uint8_t(x >> 16)), v.push_back(uint8_t(x >> 8)), v.push_back(uint8_t(x));
}

void chunk(std::ofstream& f, const char* type, const std::vector<uint8_t>& data)
{
    std::vector<uint8_t> buf;
    put32(buf, uint32_t(data.size()));
    buf.insert(buf.end(), type, type + 4);
    buf.insert(buf.end(), data.begin(), data.end());
    put32(buf, crc32(buf.data() + 4, buf.size() - 4));
    f.write(reinterpret_cast<const char*>(buf.data()), std::streamsize(buf.size()));
}

}  // namespace

bool writePng(const std::string& path, const Image& img)
{
    std::ofstream f(path, std::ios::binary);
    if (!f)
        return false;
    static const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    f.write(reinterpret_cast<const char*>(sig), 8);

    std::vector<uint8_t> ihdr;
    put32(ihdr, uint32_t(img.width));
    put32(ihdr, uint32_t(img.height));
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0});  // 8-bit RGBA, no interlace
    chunk(f, "IHDR", ihdr);

    // Raw scanlines (filter byte 0), top row first.
    std::vector<uint8_t> raw;
    raw.reserve(size_t(img.height) * (img.width * 4 + 1));
    for (int y = img.height - 1; y >= 0; --y) {
        raw.push_back(0);
        raw.insert(raw.end(), img.at(0, y), img.at(0, y) + img.width * 4);
    }

    // zlib stream made of stored (uncompressed) deflate blocks.
    std::vector<uint8_t> z = {0x78, 0x01};
    for (size_t pos = 0; pos < raw.size() || pos == 0;) {
        const size_t n = std::min<size_t>(65535, raw.size() - pos);
        const bool last = pos + n >= raw.size();
        z.push_back(last ? 1 : 0);
        z.push_back(uint8_t(n)), z.push_back(uint8_t(n >> 8));
        z.push_back(uint8_t(~n)), z.push_back(uint8_t(~n >> 8));
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
        pos += n;
        if (last)
            break;
    }
    uint32_t a = 1, b = 0;
    for (uint8_t c : raw)
        a = (a + c) % 65521, b = (b + a) % 65521;
    put32(z, (b << 16) | a);
    chunk(f, "IDAT", z);
    chunk(f, "IEND", {});
    return bool(f);
}

static float smoothstep(float e0, float e1, float x)
{
    const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

Image makeToonTexture(ToonPreset preset, int size)
{
    Image img(size, size);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            const float u = (x + 0.5f) / size;  // tone: n.l
            const float v = (y + 0.5f) / size;  // detail D
            // Three hard tone bands, slightly softened to avoid aliasing.
            const float bands = 0.35f + 0.3f * smoothstep(0.22f, 0.26f, u) + 0.35f * smoothstep(0.58f, 0.62f, u);
            // Low detail flattens the bands towards a single mid tone (the X-Toon LOD idea).
            const float tone = 0.72f + (bands - 0.72f) * v;
            glm::vec4 c(tone, tone, tone, 1.0f);
            if (preset == ToonPreset::Backlight) {
                const float rim = 1.0f - smoothstep(0.12f, 0.2f, v);
                c = glm::mix(glm::vec4(bands, bands, bands, 1.0f), glm::vec4(1.0f, 0.78f, 0.42f, 0.0f), rim);
            } else if (preset == ToonPreset::Highlight) {
                const float spot = smoothstep(0.86f, 0.9f, v) * smoothstep(0.2f, 0.3f, u);
                c = glm::mix(glm::vec4(bands, bands, bands, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.0f), spot);
            }
            img.set(x, y, c);
        }
    return img;
}

}  // namespace svo
