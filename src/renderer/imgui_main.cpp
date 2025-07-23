#include <iostream>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "main_window.hpp"
#include "visual_board.hpp"
#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

constexpr auto VSYNC_HINT{0}; // 1 for vsync, 0 for no vsync

namespace renderer {
    enum FrameAction {
        FRAME_CONTINUE,
        FRAME_EXIT,
        FRAME_SLEEP
    };

    static void
    glfw_error_callback(const int32_t error, const char *description) {
        (void) error; // Unused parameter
        (void) description; // Unused parameter
    }


    int
    imgui_initialization(GLFWwindow **window) {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return 1;

        const char *glsl_version = "#version 430 core";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);


        // Create window with graphics context
        const float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
        *window = glfwCreateWindow(static_cast<int32_t>(1920 * main_scale),
                                   static_cast<int32_t>(1080 * main_scale),
                                   "Chess", nullptr, nullptr);
        if (*window == nullptr)
            return 1;
        glfwMakeContextCurrent(*window);
        glfwSwapInterval(VSYNC_HINT); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

        ImGui::StyleColorsDark();
        // Setup scaling
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(main_scale);
        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
        style.FontScaleDpi = main_scale;

        if (!ImGui_ImplGlfw_InitForOpenGL(*window, true) || !ImGui_ImplOpenGL3_Init(glsl_version))
            return 2;

        return 0;
    }

    void
    imgui_shutdown(GLFWwindow *window) {
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    FrameAction
    frame_begin(GLFWwindow *window) {
        if (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
                ImGui_ImplGlfw_Sleep(10);
                return FRAME_SLEEP;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            return FRAME_CONTINUE;
        }
        return FRAME_EXIT;
    }

    void
    frame_end(GLFWwindow *window) {
        static constexpr auto clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        int32_t display_w;
        int32_t display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                     clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    int
    render() {
        GLFWwindow *window = nullptr;
        if (imgui_initialization(&window) != 0) {
            return 1;
        }


        if (!load_board_resources(std::filesystem::current_path().parent_path().parent_path() / "res")) {
            imgui_shutdown(window);
            std::cerr << "Failed to load resources." << std::endl;
            return 2;
        }

        MainWindow main_win;
        while (true) {
            const FrameAction frame_action = frame_begin(window);
            if (frame_action == FRAME_EXIT) {
                break;
            }
            if (frame_action == FRAME_SLEEP) {
                continue;
            }

            main_win.render();

            frame_end(window);
        }

        release_board_resources();
        imgui_shutdown(window);
        return 0;
    }
}
