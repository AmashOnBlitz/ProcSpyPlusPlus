#include "pch.h"
#include "WindowManager.h"

#define ERRSTR "[Error] : "

GLFWwindow* WindowManager::InitWindow(int width, int height, const char* title) {
    if (!glfwInit()) {
        std::cout << ERRSTR << "GLFW Init Failed!\n";
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        std::cout << ERRSTR << "Cannot Create OpenGL GLFW Window!\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);    

    if (!gladLoadGL()) {
        std::cout << ERRSTR << "Failed to initialize GLAD!\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }

    int fb_w, fb_h;
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    glViewport(0, 0, fb_w, fb_h);

    return window;
}

void WindowManager::UpdateViewport(GLFWwindow* window, int& last_fb_w, int& last_fb_h) {
    int fb_w, fb_h;
    glfwGetFramebufferSize(window, &fb_w, &fb_h);
    if (fb_w != last_fb_w || fb_h != last_fb_h) {
        glViewport(0, 0, fb_w, fb_h);
        last_fb_w = fb_w;
        last_fb_h = fb_h;
    }
}