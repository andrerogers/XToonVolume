#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>

#include "image.h"
#include "render.h"

namespace svo {

// OpenGL 4.5 compute renderer. Two passes: render.comp (rays + shading into a
// colour image and a depth/id G-buffer) and outline.comp (edges into the final
// image), then a blit to the window.
class GlRenderer {
public:
    ~GlRenderer();

    // Compiles shaders from shaderDir; safe to call again to hot-reload.
    bool loadShaders(const std::string& shaderDir, std::string& error);
    void uploadNodes(const std::vector<Node>& nodes);
    void uploadToon(const Image& toon);

    void render(const FrameData& frame, const Camera& camera, int width, int height, const RenderSettings& settings);
    void present(int framebufferWidth, int framebufferHeight);
    Image readback() const;

    // GPU time of the last-but-one frame's compute passes (query results lag a frame).
    double gpuMilliseconds() const { return gpuMs_; }

private:
    void resize(int width, int height);

    GLuint renderProgram_ = 0, outlineProgram_ = 0;
    GLuint nodeBuffer_ = 0, instanceBuffer_ = 0, bvhBuffer_ = 0;
    GLuint colorTex_ = 0, gbufferTex_ = 0, finalTex_ = 0, toonTex_ = 0;
    GLuint finalFbo_ = 0;
    GLuint timer_[2] = {0, 0};
    int timerIndex_ = 0;
    bool timerPending_[2] = {false, false};
    double gpuMs_ = 0.0;
    int width_ = 0, height_ = 0;
};

}  // namespace svo
