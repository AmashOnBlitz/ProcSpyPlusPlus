#pragma once
#include <ImGUI/imgui.h>
#include <ImGUI/imgui_impl_glfw.h>
#include <ImGUI/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

class ImGuiManager {
public:
    static void Init(GLFWwindow* window);
    static void Shutdown();
    static void NewFrame();
    static void RenderDrawData();
};
