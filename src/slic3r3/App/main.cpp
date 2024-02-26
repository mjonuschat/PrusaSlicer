#include "main.hpp"

#include <GL/glew.h>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <iostream>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "App.hpp"

namespace Slic3r::App {

struct IRenderingPlatform
{
    virtual ~IRenderingPlatform() = default;

    virtual bool init() = 0;
    virtual bool should_quit() = 0;
    virtual void poll_events() = 0;
    virtual void begin_imgui_frame() = 0;
    virtual void end_imgui_frame() = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
};

class RenderingPlatformImpl : public IRenderingPlatform
{
    static void glfw_error_callback(int error, const char *description) {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }

public:
    ~RenderingPlatformImpl() {
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }
    bool init() override {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return false;

#if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char *glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
        // GL 3.2 + GLSL 150
        const char *glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
        // GL 3.0 + GLSL 130
        const char *glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

        // Create window with graphics context
        window = glfwCreateWindow(1280, 720, "Slic3r3", nullptr, nullptr);
        if (window == nullptr)
            return false;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        const auto err = glewInit();
        if (err != GLEW_NO_ERROR) {
            std::cout << err << "\n";
            exit(EXIT_FAILURE);
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
        ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
#endif
        ImGui_ImplOpenGL3_Init(glsl_version);

        return true;
    }

    bool should_quit() override { return glfwWindowShouldClose(window); }

    void poll_events() override {
        glfwPollEvents();
        // glfwWaitEvents();
    }

    void begin_imgui_frame() override {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void end_imgui_frame() override {
        // Rendering
        ImGui::Render();
    }

    void begin_frame() override {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        glClearColor(
            clear_color.x * clear_color.w, clear_color.y * clear_color.w,
            clear_color.z * clear_color.w, clear_color.w
        );
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void end_frame() override {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

private:
    GLFWwindow *window{nullptr};
};

int slic3r_main(int argc, char **argv) {
    std::unique_ptr<IRenderingPlatform> rendering_platform = std::make_unique<RenderingPlatformImpl>(
    );
    App app;

    rendering_platform->init();

    app.init(argc, argv);

    //  Main loop
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a
    // fopen() of the imgui.ini file. You may manually call LoadIniSettingsFromMemory() to load
    // settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!rendering_platform->should_quit())
#endif
    {
        rendering_platform->poll_events();
        rendering_platform->begin_imgui_frame();
        app.render_imgui();
        rendering_platform->end_imgui_frame();

        rendering_platform->begin_frame();
        app.render();
        rendering_platform->end_frame();
    }

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    return 0;
}

} // namespace Slic3r::App
