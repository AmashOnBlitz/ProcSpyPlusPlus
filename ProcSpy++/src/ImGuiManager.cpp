#include "pch.h"
#include "ImGuiManager.h"

void ImGuiManager::Init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); 
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 14.0f);
    if (font) io.FontDefault = font;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.ChildRounding = 4.0f;
    style.PopupRounding = 6.0f;

    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 5);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.ScrollbarSize = 10.0f;
    style.GrabMinSize = 8.0f;
    style.IndentSpacing = 16.0f;

    ImVec4* c = style.Colors;

    c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.14f, 1.0f);
    c[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);

    c[ImGuiCol_Border] = ImVec4(0.18f, 0.22f, 0.28f, 1.0f);
    c[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);

    c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.13f, 0.16f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.17f, 0.21f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.20f, 0.25f, 1.0f);

    c[ImGuiCol_TitleBg] = ImVec4(0.05f, 0.06f, 0.08f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.05f, 0.06f, 0.08f, 1.0f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.06f, 0.08f, 1.0f);

    c[ImGuiCol_MenuBarBg] = ImVec4(0.06f, 0.07f, 0.09f, 1.0f);

    c[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.18f, 0.85f, 0.65f, 0.40f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.85f, 0.65f, 0.65f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.18f, 0.85f, 0.65f, 0.90f);

    c[ImGuiCol_CheckMark] = ImVec4(0.18f, 0.85f, 0.65f, 1.0f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.18f, 0.85f, 0.65f, 0.70f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.18f, 0.85f, 0.65f, 1.0f);

    c[ImGuiCol_Button] = ImVec4(0.10f, 0.60f, 0.45f, 0.85f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.14f, 0.75f, 0.56f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.08f, 0.50f, 0.38f, 1.0f);

    c[ImGuiCol_Header] = ImVec4(0.18f, 0.85f, 0.65f, 0.18f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.85f, 0.65f, 0.10f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.85f, 0.65f, 0.28f);

    c[ImGuiCol_Separator] = ImVec4(0.18f, 0.22f, 0.28f, 1.0f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.18f, 0.85f, 0.65f, 0.50f);
    c[ImGuiCol_SeparatorActive] = ImVec4(0.18f, 0.85f, 0.65f, 1.0f);

    c[ImGuiCol_ResizeGrip] = ImVec4(0.18f, 0.85f, 0.65f, 0.20f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(0.18f, 0.85f, 0.65f, 0.50f);
    c[ImGuiCol_ResizeGripActive] = ImVec4(0.18f, 0.85f, 0.65f, 0.80f);

    c[ImGuiCol_Text] = ImVec4(0.82f, 0.86f, 0.90f, 1.0f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.30f, 0.35f, 0.40f, 1.0f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.18f, 0.85f, 0.65f, 0.25f);

    c[ImGuiCol_Tab] = ImVec4(0.10f, 0.11f, 0.14f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.85f, 0.65f, 0.25f);
    c[ImGuiCol_TabActive] = ImVec4(0.14f, 0.75f, 0.56f, 0.35f);
    c[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
    c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.13f, 0.16f, 1.0f);

    c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.65f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void ImGuiManager::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiManager::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::RenderDrawData() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}