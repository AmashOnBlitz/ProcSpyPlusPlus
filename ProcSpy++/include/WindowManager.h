#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

class WindowManager {
public:
    static GLFWwindow* InitWindow(int width, int height, const char* title);
    static void UpdateViewport(GLFWwindow* window, int& last_fb_w, int& last_fb_h);
};