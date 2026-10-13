#include "gl_renderer.h"

#include <fstream>
#include <sstream>

namespace svo {

namespace {

bool readFile(const std::string& path, std::string& out)
{
    std::ifstream f(path);
    if (!f)
        return false;
    std::stringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

// GLSL has no #include, so the shared code is prepended to each pass.
GLuint compileCompute(const std::string& dir, const char* file, std::string& error)
{
    std::string common, body;
    if (!readFile(dir + "/svo.glsl", common) || !readFile(dir + "/" + file, body)) {
        error = "cannot read shaders from " + dir;
        return 0;
    }
    const std::string src = "#version 450 core\n#line 1 1\n" + common + "\n#line 1 2\n" + body;
    const char* text = src.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        error = std::string(file) + " (source 1 = svo.glsl, 2 = " + file + "):\n" + log;
        glDeleteShader(shader);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        error = std::string(file) + " link: " + log;
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void setInt(GLuint p, const char* name, int v) { glProgramUniform1i(p, glGetUniformLocation(p, name), v); }
void setFloat(GLuint p, const char* name, float v) { glProgramUniform1f(p, glGetUniformLocation(p, name), v); }
void setVec3(GLuint p, const char* name, glm::vec3 v) { glProgramUniform3f(p, glGetUniformLocation(p, name), v.x, v.y, v.z); }
void setIVec2(GLuint p, const char* name, int x, int y) { glProgramUniform2i(p, glGetUniformLocation(p, name), x, y); }

void uploadBuffer(GLuint& buffer, const void* data, size_t bytes)
{
    if (!buffer)
        glCreateBuffers(1, &buffer);
    // Never allocate zero bytes: an unbacked SSBO binding is an error on some drivers.
    static const uint32_t dummy[16] = {};
    glNamedBufferData(buffer, GLsizeiptr(bytes ? bytes : sizeof(dummy)), bytes ? data : dummy, GL_DYNAMIC_DRAW);
}

}  // namespace

GlRenderer::~GlRenderer()
{
    glDeleteProgram(renderProgram_);
    glDeleteProgram(outlineProgram_);
    GLuint buffers[] = {nodeBuffer_, instanceBuffer_, bvhBuffer_};
    glDeleteBuffers(3, buffers);
    GLuint textures[] = {colorTex_, gbufferTex_, finalTex_, toonTex_};
    glDeleteTextures(4, textures);
    glDeleteFramebuffers(1, &finalFbo_);
    glDeleteQueries(2, timer_);
}

bool GlRenderer::loadShaders(const std::string& dir, std::string& error)
{
    GLuint render = compileCompute(dir, "render.comp", error);
    if (!render)
        return false;
    GLuint outline = compileCompute(dir, "outline.comp", error);
    if (!outline) {
        glDeleteProgram(render);
        return false;
    }
    glDeleteProgram(renderProgram_);
    glDeleteProgram(outlineProgram_);
    renderProgram_ = render, outlineProgram_ = outline;
    if (!timer_[0])
        glCreateQueries(GL_TIME_ELAPSED, 2, timer_);
    return true;
}

void GlRenderer::uploadNodes(const std::vector<Node>& nodes)
{
    uploadBuffer(nodeBuffer_, nodes.data(), nodes.size() * sizeof(Node));
}

void GlRenderer::uploadToon(const Image& toon)
{
    glDeleteTextures(1, &toonTex_);
    glCreateTextures(GL_TEXTURE_2D, 1, &toonTex_);
    glTextureStorage2D(toonTex_, 1, GL_RGBA8, toon.width, toon.height);
    glTextureSubImage2D(toonTex_, 0, 0, 0, toon.width, toon.height, GL_RGBA, GL_UNSIGNED_BYTE, toon.rgba.data());
    // Clamp, not repeat: the original wrapped out-of-range tone/detail values into false bands.
    glTextureParameteri(toonTex_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(toonTex_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(toonTex_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(toonTex_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void GlRenderer::resize(int width, int height)
{
    if (width == width_ && height == height_)
        return;
    width_ = width, height_ = height;
    GLuint textures[] = {colorTex_, gbufferTex_, finalTex_};
    glDeleteTextures(3, textures);
    auto make = [&](GLuint& tex, GLenum format) {
        glCreateTextures(GL_TEXTURE_2D, 1, &tex);
        glTextureStorage2D(tex, 1, format, width, height);
    };
    make(colorTex_, GL_RGBA8);
    make(gbufferTex_, GL_RG32F);
    make(finalTex_, GL_RGBA8);
    if (!finalFbo_)
        glCreateFramebuffers(1, &finalFbo_);
    glNamedFramebufferTexture(finalFbo_, GL_COLOR_ATTACHMENT0, finalTex_, 0);
}

void GlRenderer::render(const FrameData& frame, const Camera& camera, int width, int height, const RenderSettings& s)
{
    resize(width, height);
    uploadBuffer(instanceBuffer_, frame.instances.data(), frame.instances.size() * sizeof(GpuInstance));
    uploadBuffer(bvhBuffer_, frame.bvh.data(), frame.bvh.size() * sizeof(BvhNode));

    // Read the timer from two frames ago so we never stall waiting for the GPU.
    const int slot = timerIndex_;
    if (timerPending_[slot]) {
        GLuint64 ns = 0;
        glGetQueryObjectui64v(timer_[slot], GL_QUERY_RESULT, &ns);
        gpuMs_ = double(ns) * 1e-6;
    }
    glBeginQuery(GL_TIME_ELAPSED, timer_[slot]);

    const GLuint p = renderProgram_;
    setInt(p, "uInstanceCount", int(frame.instances.size()));
    setFloat(p, "uLodBias", s.lodBias);
    setInt(p, "uMaxSteps", s.maxSteps);
    setVec3(p, "uCamPos", camera.position);
    setVec3(p, "uCamForward", camera.forward);
    setVec3(p, "uCamRight", camera.right);
    setVec3(p, "uCamUp", camera.up);
    setFloat(p, "uTanHalfFovY", camera.tanHalfFovY);
    setFloat(p, "uAspect", camera.aspect);
    setIVec2(p, "uSize", width, height);
    setInt(p, "uView", int(s.view));
    setInt(p, "uDetail", int(s.detail));
    setFloat(p, "uZMin", s.zMin);
    setFloat(p, "uR", s.r);
    setFloat(p, "uFocus", s.focus);
    setFloat(p, "uFocusWidth", s.focusWidth);
    setFloat(p, "uBacklightR", s.backlightR);
    setFloat(p, "uHighlightS", s.highlightS);
    setFloat(p, "uAlbedoMix", s.albedoMix);
    setVec3(p, "uLightDir", s.lightDir);
    setInt(p, "uSmoothNormals", s.smoothNormals);
    setInt(p, "uShadows", s.shadows);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, nodeBuffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, instanceBuffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, bvhBuffer_);
    glBindTextureUnit(0, toonTex_);
    glBindImageTexture(0, colorTex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glBindImageTexture(1, gbufferTex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    glUseProgram(p);
    const GLuint gx = GLuint((width + 7) / 8), gy = GLuint((height + 7) / 8);
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    const GLuint q = outlineProgram_;
    setIVec2(q, "uSize", width, height);
    setInt(q, "uOutlines", s.outlines && s.view == View::XToon);
    setFloat(q, "uOutlineDepth", s.outlineDepth);
    glBindImageTexture(0, colorTex_, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
    glBindImageTexture(1, gbufferTex_, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(2, finalTex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glUseProgram(q);
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
    glUseProgram(0);

    glEndQuery(GL_TIME_ELAPSED);
    timerPending_[slot] = true;
    timerIndex_ = 1 - slot;
}

void GlRenderer::present(int fbWidth, int fbHeight)
{
    glBlitNamedFramebuffer(finalFbo_, 0, 0, 0, width_, height_, 0, 0, fbWidth, fbHeight, GL_COLOR_BUFFER_BIT,
                           GL_LINEAR);
}

Image GlRenderer::readback() const
{
    Image img(width_, height_);
    glGetTextureImage(finalTex_, 0, GL_RGBA, GL_UNSIGNED_BYTE, GLsizei(img.rgba.size()), img.rgba.data());
    return img;
}

}  // namespace svo
