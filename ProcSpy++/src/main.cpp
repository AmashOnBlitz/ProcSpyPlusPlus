#include "pch.h"
#include "WindowManager.h"
#include "ImGuiManager.h"
#include "Render.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    GLFWwindow* window = WindowManager::InitWindow(1200, 600, "ProcSpy++");
    if (!window) return 1;

    int last_fb_w = 0, last_fb_h = 0;
    ImGuiManager::Init(window);

    while (!glfwWindowShouldClose(window)) {
        glfwWaitEventsTimeout(0.05);
        WindowManager::UpdateViewport(window, last_fb_w, last_fb_h);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGuiManager::NewFrame();

        int win_w, win_h;
        glfwGetWindowSize(window, &win_w, &win_h);

        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2((float)win_w, (float)win_h);
        io.DisplayFramebufferScale = ImVec2((float)last_fb_w / win_w, (float)last_fb_h / win_h);
        io.FontGlobalScale = std::min((float)win_w / 800.0f, (float)win_h / 600.0f);

        ImGui::SetNextWindowPos(ImVec2(0, 0));  
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_MenuBar |
            ImGuiWindowFlags_NoScrollbar;

        Render(flags);

        ImGuiManager::RenderDrawData();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGuiManager::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
