#include "pch.h"
#include "WindowManager.h"
#include "ImGuiManager.h"
#include "Render.h"
#include "../Resources/resource.h"
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    GLFWwindow* window = WindowManager::InitWindow(1440, 860, "ProcSpy++");
    if (!window) return 1;
    HWND hwnd = glfwGetWin32Window(window);

    HICON iconBig = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_ICON1),
        IMAGE_ICON,
        32,
        32,
        LR_DEFAULTCOLOR
    );

    HICON iconSmall = (HICON)LoadImage(
        hInstance,
        MAKEINTRESOURCE(IDI_ICON1),
        IMAGE_ICON,
        16,
        16,
        LR_DEFAULTCOLOR
    );

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)iconBig);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)iconSmall);

    int last_fb_w = 0, last_fb_h = 0;
    ImGuiManager::Init(window);
    Render_Init();

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
        io.FontGlobalScale = 1.0f;

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
    CleanupAPIHooks();
    EjectAllSync();
    Render_Shutdown();
    ImGuiManager::Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}