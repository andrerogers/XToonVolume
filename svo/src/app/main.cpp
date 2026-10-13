// Interactive viewer: animated crowd of sparse voxel octree instances, ray cast
// on the GPU with X-Toon shading.
//
//   left drag: orbit    right drag: pan    wheel: zoom
//   space: play/pause   R: reload shaders  F12: screenshot   Esc: quit

#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "gpu/gl_renderer.h"
#include "tools/options.h"

using namespace svo;
using Clock = std::chrono::steady_clock;

namespace {

struct Orbit {
    glm::vec3 target{0.0f, 1.0f, 0.0f};
    float yaw = 0.0f, pitch = 0.28f, distance = 11.5f;

    Camera camera(float aspect) const
    {
        const glm::vec3 offset(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch));
        return Camera::lookAt(target + offset * distance, target, 45.0f, aspect);
    }
};

const char* kToonNames[] = {"bands", "backlight", "highlight", "thesis 01", "thesis 02", "thesis 03", "thesis 04"};

bool loadToonByIndex(int index, Image& out, std::string& error)
{
    if (index < 3)
        return loadToon(kToonNames[index], out, error);
    return loadTga(std::string(SVO_ASSET_DIR) + "/toon/transferTex_0" + std::to_string(index - 2) + ".tga", out, error);
}

// Fraction of pixels whose largest channel difference exceeds `tolerance` (0-255).
double imageDifference(const Image& a, const Image& b, int tolerance)
{
    if (a.width != b.width || a.height != b.height)
        return 1.0;
    size_t bad = 0;
    for (size_t i = 0; i < a.rgba.size(); i += 4) {
        int d = 0;
        for (int c = 0; c < 3; ++c)
            d = std::max(d, std::abs(int(a.rgba[i + c]) - int(b.rgba[i + c])));
        bad += d > tolerance;
    }
    return double(bad) / double(a.rgba.size() / 4);
}

void glfwError(int code, const char* message) { std::fprintf(stderr, "GLFW error %d: %s\n", code, message); }

}  // namespace

int main(int argc, char** argv)
{
    CommonOptions o;
    o.width = 1280, o.height = 720;
    std::string screenshot;
    bool compare = false, hidden = false;
    int warmupFrames = 3;
    for (int i = 1; i < argc; ++i) {
        if (parseCommon(argc, argv, i, o))
            continue;
        if (!std::strcmp(argv[i], "--screenshot") && i + 1 < argc) screenshot = argv[++i];
        else if (!std::strcmp(argv[i], "--compare")) compare = true;
        else if (!std::strcmp(argv[i], "--hidden")) hidden = true;
        else {
            std::printf("usage: xtoon_svo [options]\n%s"
                        "  --screenshot FILE.png    render one frame at --time, save it and exit\n"
                        "  --compare                with --screenshot: also render on the CPU and diff\n"
                        "  --hidden                 do not show the window (for automation)\n",
                        commonHelp());
            return std::strcmp(argv[i], "--help") ? 2 : 0;
        }
    }

    glfwSetErrorCallback(glfwError);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if (hidden)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(o.width, o.height, "X-Toon Sparse Voxel Octrees", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Could not create an OpenGL 4.5 core context. Check `glxinfo -B`.\n");
        return 1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "Failed to load OpenGL functions\n");
        return 1;
    }
    glfwSwapInterval(1);
    std::printf("OpenGL %s on %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450 core");

    std::string error;
    Demo demo;
    auto t0 = Clock::now();
    if (!demo.build(o.demo, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    double buildSeconds = std::chrono::duration<double>(Clock::now() - t0).count();
    std::vector<Node> nodes = demo.scene.packNodes();

    // Owned by a pointer so it can be destroyed while the GL context still exists.
    auto rendererOwner = std::make_unique<GlRenderer>();
    GlRenderer& renderer = *rendererOwner;
    if (!renderer.loadShaders(SVO_SHADER_DIR, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    renderer.uploadNodes(nodes);
    Image toon;
    int toonIndex = 0;
    for (int i = 0; i < 3; ++i)
        if (o.toon == kToonNames[i])
            toonIndex = i;
    if (!loadToon(o.toon, toon, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    renderer.uploadToon(toon);

    Orbit orbit;
    RenderSettings& rs = o.render;
    float time = o.time, renderScale = 1.0f;
    bool playing = screenshot.empty();
    float lightAzimuth = 0.96f, lightElevation = 0.93f;
    std::string shaderError;
    double prepareMs = 0.0;
    int frameCount = 0;
    auto last = Clock::now();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const auto now = Clock::now();
        const float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        if (playing)
            time += dt;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        // Camera and shortcuts.
        if (!io.WantCaptureMouse) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                orbit.yaw -= io.MouseDelta.x * 0.006f;
                orbit.pitch = glm::clamp(orbit.pitch + io.MouseDelta.y * 0.006f, -1.5f, 1.5f);
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
                const Camera c = orbit.camera(1.0f);
                orbit.target += (-c.right * io.MouseDelta.x + c.up * io.MouseDelta.y) * orbit.distance * 0.0015f;
            }
            orbit.distance = glm::clamp(orbit.distance * std::pow(0.9f, io.MouseWheel), 0.5f, 200.0f);
        }
        bool takeScreenshot = false;
        if (!io.WantCaptureKeyboard) {
            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) glfwSetWindowShouldClose(window, 1);
            if (ImGui::IsKeyPressed(ImGuiKey_Space)) playing = !playing;
            if (ImGui::IsKeyPressed(ImGuiKey_F12)) takeScreenshot = true;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                shaderError.clear();
                renderer.loadShaders(SVO_SHADER_DIR, shaderError);
            }
        }

        // UI.
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("X-Toon SVO");
        ImGui::Text("%.1f fps   GPU %.2f ms   BVH+upload %.2f ms", io.Framerate, renderer.gpuMilliseconds(), prepareMs);
        ImGui::Text("%zu instances, %zu models, %zu nodes (%.1f MB)", demo.scene.instances.size(),
                    demo.scene.models.size(), nodes.size(), double(nodes.size() * sizeof(Node)) / 1e6);
        ImGui::Text("scene built in %.2f s", buildSeconds);
        if (!shaderError.empty())
            ImGui::TextColored(ImVec4(1, 0.4f, 0.3f, 1), "%s", shaderError.c_str());

        if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Play (space)", &playing);
            ImGui::SliderFloat("Time", &time, 0.0f, 60.0f);
            ImGui::SliderFloat("Walk speed", &demo.speed, 0.0f, 3.0f);
            ImGui::SliderFloat("Centre spin", &demo.centrepieceSpin, -1.0f, 1.0f);
            ImGui::SliderInt("Walkers", &o.demo.walkers, 0, 200);
            ImGui::SliderInt("Limb resolution", &o.demo.partResolution, 8, 256);
            if (ImGui::Button("Rebuild scene")) {
                t0 = Clock::now();
                if (demo.build(o.demo, error)) {
                    buildSeconds = std::chrono::duration<double>(Clock::now() - t0).count();
                    nodes = demo.scene.packNodes();
                    renderer.uploadNodes(nodes);
                }
            }
        }
        if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* views[] = {"X-Toon", "Albedo", "Normals", "Octree level", "Traversal cost", "Detail (D)"};
            ImGui::Combo("View", reinterpret_cast<int*>(&rs.view), views, IM_ARRAYSIZE(views));
            ImGui::SliderFloat("Render scale", &renderScale, 0.25f, 1.0f);
            ImGui::SliderFloat("LOD bias (px)", &rs.lodBias, 0.0f, 8.0f);
            ImGui::Checkbox("Smooth normals", &rs.smoothNormals);
            ImGui::SameLine();
            ImGui::Checkbox("Shadows", &rs.shadows);
            ImGui::SameLine();
            ImGui::Checkbox("Outlines", &rs.outlines);
            ImGui::SliderFloat("Light azimuth", &lightAzimuth, -3.14f, 3.14f);
            ImGui::SliderFloat("Light elevation", &lightElevation, 0.05f, 1.55f);
        }
        if (ImGui::CollapsingHeader("X-Toon", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* modes[] = {"Off", "Depth (LOD)", "Focus (DOF)", "Backlight |n.v|^r", "Highlight |v.r|^s",
                                   "Octree level"};
            ImGui::Combo("Detail", reinterpret_cast<int*>(&rs.detail), modes, IM_ARRAYSIZE(modes));
            if (ImGui::Combo("Toon texture", &toonIndex, kToonNames, IM_ARRAYSIZE(kToonNames))) {
                if (loadToonByIndex(toonIndex, toon, error))
                    renderer.uploadToon(toon);
            }
            ImGui::SliderFloat("Colour mix", &rs.albedoMix, 0.0f, 1.0f);
            switch (rs.detail) {
            case xtoon::DetailMode::Depth:
                ImGui::SliderFloat("z min", &rs.zMin, 0.5f, 40.0f);
                ImGui::SliderFloat("r (coarseness)", &rs.r, 1.0f, 20.0f);
                break;
            case xtoon::DetailMode::Focus:
                ImGui::SliderFloat("Focus distance", &rs.focus, 0.5f, 40.0f);
                ImGui::SliderFloat("In-focus width", &rs.focusWidth, 0.1f, 10.0f);
                ImGui::SliderFloat("r (coarseness)", &rs.r, 1.0f, 20.0f);
                break;
            case xtoon::DetailMode::Backlight: ImGui::SliderFloat("r", &rs.backlightR, 0.1f, 10.0f); break;
            case xtoon::DetailMode::Highlight: ImGui::SliderFloat("s (shininess)", &rs.highlightS, 1.0f, 128.0f); break;
            default: break;
            }
        }
        ImGui::End();
        rs.lightDir = glm::vec3(std::cos(lightElevation) * std::sin(lightAzimuth), std::sin(lightElevation),
                                std::cos(lightElevation) * std::cos(lightAzimuth));

        // Animate, rebuild the top-level BVH, render.
        int fbw, fbh;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        const int rw = std::max(1, int(float(fbw) * renderScale)), rh = std::max(1, int(float(fbh) * renderScale));
        const Camera camera = orbit.camera(float(rw) / float(rh));
        const auto p0 = Clock::now();
        demo.update(time);
        const FrameData frame = demo.scene.prepareFrame();
        prepareMs = std::chrono::duration<double, std::milli>(Clock::now() - p0).count();
        renderer.render(frame, camera, rw, rh, rs);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, fbw, fbh);
        renderer.present(fbw, fbh);
        ImGui::Render();
        if (screenshot.empty())  // keep automated captures free of UI
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        if (takeScreenshot) {
            char name[64];
            std::snprintf(name, sizeof(name), "xtoon_svo_%04d.png", frameCount);
            writePng(name, renderer.readback());
            std::printf("saved %s\n", name);
        }
        if (!screenshot.empty() && ++frameCount >= warmupFrames) {
            const Image gpu = renderer.readback();
            writePng(screenshot, gpu);
            std::printf("saved %s (%dx%d), GPU %.2f ms\n", screenshot.c_str(), gpu.width, gpu.height,
                        renderer.gpuMilliseconds());
            int status = 0;
            if (compare) {
                const Image cpu = renderCpu(frame, nodes, camera, rw, rh, rs, toon);
                std::string cpuPath = screenshot;
                cpuPath.insert(cpuPath.rfind(".png") == std::string::npos ? cpuPath.size() : cpuPath.rfind(".png"), "_cpu");
                writePng(cpuPath, cpu);
                const double diff = imageDifference(gpu, cpu, 24);
                std::printf("GPU vs CPU: %.3f%% of pixels differ by more than 24/255\n", diff * 100.0);
                status = diff < 0.01 ? 0 : 3;
            }
            rendererOwner.reset();
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
            glfwTerminate();
            return status;
        }
        if (screenshot.empty())
            ++frameCount;
    }

    rendererOwner.reset();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
