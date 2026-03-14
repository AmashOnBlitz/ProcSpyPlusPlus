#pragma once
#include <ImGUI/imgui.h>

void Render(ImGuiWindowFlags flags);
void CleanupAPIHooks();
void EjectAllSync();
void Render_Shutdown();
void Render_Init();