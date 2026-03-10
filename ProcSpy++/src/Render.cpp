#include "pch.h"
#include "Render.h"
#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>
#include <tlhelp32.h>
#include <Injector.h>

struct ProcessEntry {
    DWORD       pid;
    std::string name;
};

struct InjectedEntry {
    DWORD       pid;
    std::string name;
};

struct FailedEntry {
    DWORD       pid;
    std::string name;
};

static std::vector<ProcessEntry>  g_processList;
static std::vector<uint8_t>       g_processSelected;
static std::vector<InjectedEntry> g_injectedList;

static std::vector<FailedEntry>   g_injectFailed;
static std::vector<FailedEntry>   g_ejectFailed;

static char g_processFilter[128]  = "";
static bool g_showInjectModal     = false;
static bool g_showEjectModal      = false;
static int  g_ejectSelectedIdx    = -1;
static int  g_injectedSelectedIdx = -1;

static bool InjectProcess(DWORD pid, const std::string& name) {
    return Injector::Inject(pid,name);
}

static bool EjectProcess(DWORD pid, const std::string& name) {
    return Injector::Eject(pid,name);
}

static void RefreshProcessList() {
    g_processList.clear();
    g_processSelected.clear();

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            ProcessEntry e;
            e.pid = pe.th32ProcessID;
            int sz = WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, nullptr, 0, nullptr, nullptr);
            std::string s(sz, 0);
            WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, &s[0], sz, nullptr, nullptr);
            if (!s.empty() && s.back() == '\0') s.pop_back();
            e.name = s;
            g_processList.push_back(e);
            g_processSelected.push_back(0);
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
}

static int CountSelected() {
    int n = 0;
    for (uint8_t b : g_processSelected) if (b) n++;
    return n;
}

static void DoInjectSelected() {
    g_injectFailed.clear();
    for (int i = 0; i < (int)g_processList.size(); i++) {
        if (!g_processSelected[i]) continue;
        bool already = false;
        for (auto& inj : g_injectedList)
            if (inj.pid == g_processList[i].pid) { already = true; break; }
        if (already) continue;
        if (InjectProcess(g_processList[i].pid, g_processList[i].name))
            g_injectedList.push_back({ g_processList[i].pid, g_processList[i].name });
        else
            g_injectFailed.push_back({ g_processList[i].pid, g_processList[i].name });
    }
}

static void RenderInjectModal() {
    if (!g_showInjectModal) return;

    float modalH = g_injectFailed.empty() ? 420.0f : 490.0f;
    ImGui::SetNextWindowSize(ImVec2(500, modalH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleColor(ImGuiCol_PopupBg,       ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(0.18f, 0.85f, 0.65f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,    ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,  ImVec4(0.18f, 0.85f, 0.65f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,      ImVec2(8, 5));

    ImGui::OpenPopup("##InjectPicker");
    if (ImGui::BeginPopupModal("##InjectPicker", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::SetCursorPosX(14);
        ImGui::Text("SELECT TARGET PROCESSES");
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 26);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f,  0.0f,  0.0f,  0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f,  0.2f,  0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.85f, 0.2f,  0.2f,  0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.7f,  0.7f,  0.7f,  1.0f));
        if (ImGui::Button("x##iclose", ImVec2(22, 22))) {
            g_injectFailed.clear();
            g_showInjectModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 2));

        auto GhostBtn = [](const char* label, float w) -> bool {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.25f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.18f, 0.85f, 0.65f, 0.45f));
            ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
            bool r = ImGui::Button(label, ImVec2(w, 0));
            ImGui::PopStyleColor(4);
            return r;
        };

        float availW = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(availW - 190);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.90f, 1.0f));
        ImGui::InputTextWithHint("##ifilter", "Filter processes...", g_processFilter, sizeof(g_processFilter));
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (GhostBtn("Refresh", 68)) RefreshProcessList();
        ImGui::SameLine();
        if (GhostBtn("All", 36)) {
            std::string fl = g_processFilter;
            for (auto& ch : fl) ch = (char)tolower(ch);
            for (int i = 0; i < (int)g_processList.size(); i++) {
                if (fl.empty()) { g_processSelected[i] = 1; continue; }
                std::string nl = g_processList[i].name;
                for (auto& ch : nl) ch = (char)tolower(ch);
                if (nl.find(fl) != std::string::npos) g_processSelected[i] = 1;
            }
        }
        ImGui::SameLine();
        if (GhostBtn("None", 40)) {
            for (auto& b : g_processSelected) b = 0;
        }

        ImGui::Dummy(ImVec2(0, 2));

        float listH = g_injectFailed.empty() ? 278.0f : 204.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
        ImGui::BeginChild("##iproclist", ImVec2(0, listH), true);

        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.55f, 0.60f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.18f, 0.85f, 0.65f, 0.20f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.18f, 0.85f, 0.65f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark,     ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.12f, 0.14f, 0.17f, 1.0f));

        std::string filterLow = g_processFilter;
        for (auto& ch : filterLow) ch = (char)tolower(ch);

        for (int i = 0; i < (int)g_processList.size(); i++) {
            const auto& proc = g_processList[i];
            std::string nameLow = proc.name;
            for (auto& ch : nameLow) ch = (char)tolower(ch);
            if (!filterLow.empty() && nameLow.find(filterLow) == std::string::npos) continue;

            bool checked = (g_processSelected[i] != 0);
            if (checked)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));

            char cbId[32];
            snprintf(cbId, sizeof(cbId), "##cb%d", i);
            if (ImGui::Checkbox(cbId, &checked))
                g_processSelected[i] = checked ? 1 : 0;
            ImGui::SameLine();

            char rowLabel[256];
            snprintf(rowLabel, sizeof(rowLabel), "%-38s PID: %lu",
                     proc.name.c_str(), (unsigned long)proc.pid);
            if (ImGui::Selectable(rowLabel, checked, ImGuiSelectableFlags_SpanAllColumns))
                g_processSelected[i] = checked ? 0 : 1;

            if (checked) ImGui::PopStyleColor();
        }

        ImGui::PopStyleColor(6);
        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (!g_injectFailed.empty()) {
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.35f, 0.35f, 0.9f));
            ImGui::Text("  Failed to inject (%d):", (int)g_injectFailed.size());
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.08f, 0.08f, 1.0f));
            ImGui::BeginChild("##ifailed", ImVec2(0, 56), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.30f, 0.30f, 1.0f));
            for (auto& f : g_injectFailed) {
                char fl[256];
                snprintf(fl, sizeof(fl), "  %-38s PID: %lu", f.name.c_str(), (unsigned long)f.pid);
                ImGui::TextUnformatted(fl);
            }
            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0, 4));
        int selCount = CountSelected();

        ImGui::PushStyleColor(ImGuiCol_Text, selCount > 0
            ? ImVec4(0.18f, 0.85f, 0.65f, 0.9f)
            : ImVec4(0.35f, 0.38f, 0.42f, 1.0f));
        ImGui::Text("  %d process%s selected", selCount, selCount == 1 ? "" : "es");
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 96);

        bool canConfirm = (selCount > 0);
        if (!canConfirm) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
        ImGui::PushStyleColor(ImGuiCol_Button,        canConfirm ? ImVec4(0.10f, 0.60f, 0.45f, 1.0f) : ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, canConfirm ? ImVec4(0.14f, 0.75f, 0.56f, 1.0f) : ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  canConfirm ? ImVec4(0.08f, 0.50f, 0.38f, 1.0f) : ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        char confirmLabel[48];
        snprintf(confirmLabel, sizeof(confirmLabel), "Inject (%d)", selCount);
        if (ImGui::Button(confirmLabel, ImVec2(96, 0)) && canConfirm) {
            DoInjectSelected();
            if (g_injectFailed.empty()) {
                g_showInjectModal = false;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::PopStyleColor(4);
        if (!canConfirm) ImGui::PopStyleVar();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

static void RenderEjectModal() {
    if (!g_showEjectModal) return;

    float modalH = g_ejectFailed.empty() ? 340.0f : 420.0f;
    ImGui::SetNextWindowSize(ImVec2(420, modalH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(
        ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleColor(ImGuiCol_PopupBg,       ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(0.70f, 0.20f, 0.20f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,    ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,  ImVec4(0.70f, 0.20f, 0.20f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,      ImVec2(8, 5));

    ImGui::OpenPopup("##EjectPicker");
    if (ImGui::BeginPopupModal("##EjectPicker", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.30f, 0.30f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::SetCursorPosX(14);
        ImGui::Text("EJECT");
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 26);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.0f,  0.0f,  0.0f,  0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f,  0.2f,  0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.85f, 0.2f,  0.2f,  0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.7f,  0.7f,  0.7f,  1.0f));
        if (ImGui::Button("x##eclose", ImVec2(22, 22))) {
            g_ejectFailed.clear();
            g_showEjectModal   = false;
            g_ejectSelectedIdx = -1;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.60f, 0.65f, 1.0f));
        ImGui::TextWrapped("Select a process to eject individually, or eject all at once.");
        ImGui::PopStyleColor();

        ImGui::Dummy(ImVec2(0, 6));

        float listH = g_ejectFailed.empty() ? 196.0f : 126.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
        ImGui::BeginChild("##ejectlist", ImVec2(0, listH), true);

        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.55f, 0.60f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.70f, 0.15f, 0.15f, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.70f, 0.15f, 0.15f, 0.15f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0.70f, 0.15f, 0.15f, 0.35f));

        if (g_injectedList.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.38f, 0.42f, 1.0f));
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::Text("  No injected processes");
            ImGui::PopStyleColor();
        }

        for (int i = 0; i < (int)g_injectedList.size(); i++) {
            bool sel = (g_ejectSelectedIdx == i);
            if (sel) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.35f, 0.35f, 1.0f));

            char rowLabel[256];
            snprintf(rowLabel, sizeof(rowLabel), "  %-34s PID: %lu",
                     g_injectedList[i].name.c_str(), (unsigned long)g_injectedList[i].pid);
            if (ImGui::Selectable(rowLabel, sel))
                g_ejectSelectedIdx = (g_ejectSelectedIdx == i) ? -1 : i;

            if (sel) ImGui::PopStyleColor();
        }

        ImGui::PopStyleColor(4);
        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (!g_ejectFailed.empty()) {
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.35f, 0.35f, 0.9f));
            ImGui::Text("  Failed to eject (%d):", (int)g_ejectFailed.size());
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.08f, 0.08f, 1.0f));
            ImGui::BeginChild("##efailed", ImVec2(0, 56), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.30f, 0.30f, 1.0f));
            for (auto& f : g_ejectFailed) {
                char fl[256];
                snprintf(fl, sizeof(fl), "  %-34s PID: %lu", f.name.c_str(), (unsigned long)f.pid);
                ImGui::TextUnformatted(fl);
            }
            ImGui::PopStyleColor();
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0, 6));

        bool hasInjected = !g_injectedList.empty();
        bool hasSelected = (g_ejectSelectedIdx >= 0 && g_ejectSelectedIdx < (int)g_injectedList.size());

        auto RedBtn = [](const char* lbl, float w, bool solid) -> bool {
            if (solid) {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.12f, 0.12f, 0.90f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.45f, 0.10f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f,  1.0f,  1.0f,  1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.12f, 0.12f, 0.35f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.55f, 0.12f, 0.12f, 0.55f));
                ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.80f, 0.30f, 0.30f, 1.0f));
            }
            bool r = ImGui::Button(lbl, ImVec2(w, 0));
            ImGui::PopStyleColor(4);
            return r;
        };

        if (!hasSelected) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.40f);
        if (RedBtn("Eject Selected", 122, hasSelected) && hasSelected) {
            auto& target = g_injectedList[g_ejectSelectedIdx];
            if (EjectProcess(target.pid, target.name)) {
                if (g_injectedSelectedIdx == g_ejectSelectedIdx) g_injectedSelectedIdx = -1;
                else if (g_injectedSelectedIdx > g_ejectSelectedIdx) g_injectedSelectedIdx--;
                g_injectedList.erase(g_injectedList.begin() + g_ejectSelectedIdx);
                g_ejectSelectedIdx = -1;
                g_ejectFailed.clear();
                if (g_injectedList.empty()) {
                    g_showEjectModal = false;
                    ImGui::CloseCurrentPopup();
                }
            } else {
                g_ejectFailed.clear();
                g_ejectFailed.push_back({ target.pid, target.name });
            }
        }
        if (!hasSelected) ImGui::PopStyleVar();

        ImGui::SameLine(0, 8);

        if (!hasInjected) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.40f);
        if (RedBtn("Eject All", 86, hasInjected) && hasInjected) {
            g_ejectFailed.clear();
            std::vector<InjectedEntry> remaining;
            for (auto& inj : g_injectedList) {
                if (EjectProcess(inj.pid, inj.name)) {
                    if (g_injectedSelectedIdx >= 0 &&
                        g_injectedList[g_injectedSelectedIdx].pid == inj.pid)
                        g_injectedSelectedIdx = -1;
                } else {
                    g_ejectFailed.push_back({ inj.pid, inj.name });
                    remaining.push_back(inj);
                }
            }
            g_injectedList = remaining;
            g_ejectSelectedIdx = -1;
            if (g_ejectFailed.empty()) {
                g_showEjectModal = false;
                ImGui::CloseCurrentPopup();
            }
        }
        if (!hasInjected) ImGui::PopStyleVar();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

static void RenderMenuBar() {
    ImVec2 size = ImGui::GetWindowSize();

    ImGui::PushStyleColor(ImGuiCol_MenuBarBg,     ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.10f, 0.60f, 0.45f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.75f, 0.56f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.08f, 0.50f, 0.38f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.95f, 1.0f,  0.98f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(6, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(8, 6));

    if (ImGui::BeginMenuBar()) {
        float btnW = (size.x - 48.0f) / 4.5f;

        if (ImGui::Button("  Inject All  ", ImVec2(btnW, 0))) {
            if (g_processList.empty()) RefreshProcessList();
            for (auto& b : g_processSelected) b = 1;
            DoInjectSelected();
        }
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.08f, 0.40f, 0.65f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.52f, 0.80f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.06f, 0.33f, 0.55f, 1.0f));
        if (ImGui::Button("  Selective Inject  ", ImVec2(btnW * 1.55f, 0))) {
            if (g_processList.empty()) RefreshProcessList();
            g_injectFailed.clear();
            g_showInjectModal = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        bool hasInjected = !g_injectedList.empty();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f, 0.12f, 0.12f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.45f, 0.10f, 0.10f, 1.0f));
        if (!hasInjected) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
        if (ImGui::Button("  Eject  ", ImVec2(btnW, 0)) && hasInjected) {
            g_ejectFailed.clear();
            g_showEjectModal   = true;
            g_ejectSelectedIdx = -1;
        }
        if (!hasInjected) ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        if (hasInjected) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.75f));
            ImGui::Text("  %d injected", (int)g_injectedList.size());
            ImGui::PopStyleColor();
        }

        ImGui::EndMenuBar();
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

void Render(ImGuiWindowFlags flags) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg,             ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,              ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border,               ImVec4(0.18f, 0.22f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,          ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,        ImVec4(0.18f, 0.85f, 0.65f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.65f));
    ImGui::PushStyleColor(ImGuiCol_Header,               ImVec4(0.18f, 0.85f, 0.65f, 0.18f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,        ImVec4(0.18f, 0.85f, 0.65f, 0.10f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,         ImVec4(0.18f, 0.85f, 0.65f, 0.28f));
    ImGui::PushStyleColor(ImGuiCol_Separator,            ImVec4(0.18f, 0.22f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,                 ImVec4(0.82f, 0.86f, 0.90f, 1.0f));

    ImGui::Begin("FullWindow", nullptr, flags);
    RenderMenuBar();

    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    float  statusBarH = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y + 2.0f;
    float  panelH     = windowSize.y - statusBarH - ImGui::GetStyle().ItemSpacing.y;
    float  listWidth  = windowSize.x * 0.35f;
    float  rightWidth = windowSize.x - listWidth - ImGui::GetStyle().ItemSpacing.x;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(4, 4));

    ImGui::BeginChild("InjectedList", ImVec2(listWidth, panelH), true,
                      ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.7f));
    ImGui::Dummy(ImVec2(0, 2));
    ImGui::Text("  INJECTED PROCESSES");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 2));

    if (g_injectedList.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.34f, 0.38f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Text("  No active injections");
        ImGui::PopStyleColor();
    }

    for (int i = 0; i < (int)g_injectedList.size(); i++) {
        bool sel = (g_injectedSelectedIdx == i);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));

        char rowLabel[256];
        snprintf(rowLabel, sizeof(rowLabel), "  %-26s  %lu",
                 g_injectedList[i].name.c_str(), (unsigned long)g_injectedList[i].pid);
        if (ImGui::Selectable(rowLabel, sel))
            g_injectedSelectedIdx = (g_injectedSelectedIdx == i) ? -1 : i;

        if (sel) ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::SameLine(0, 8);

    ImGui::BeginChild("ActivityPanel", ImVec2(rightWidth, panelH), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.7f));
    ImGui::Dummy(ImVec2(0, 2));
    if (g_injectedSelectedIdx >= 0 && g_injectedSelectedIdx < (int)g_injectedList.size()) {
        ImGui::Text("  ACTIVITY  -  %s  (PID %lu)",
                    g_injectedList[g_injectedSelectedIdx].name.c_str(),
                    (unsigned long)g_injectedList[g_injectedSelectedIdx].pid);
    } else {
        ImGui::Text("  ACTIVITY");
    }
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 2));

    if (g_injectedSelectedIdx < 0 || g_injectedSelectedIdx >= (int)g_injectedList.size()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.34f, 0.38f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Text("  Select an injected process to view its activity");
        ImGui::PopStyleColor();
    } else {
        // TODO: render per-process activity for:
        //   g_injectedList[g_injectedSelectedIdx].pid
        //   g_injectedList[g_injectedSelectedIdx].name
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
    ImGui::BeginChild("StatusBar", ImVec2(0, statusBarH), false);
    ImGui::Dummy(ImVec2(0, 1));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.40f, 0.45f, 1.0f));
    ImGui::Text("  Status: Ready");
    ImGui::SameLine(); ImGui::Text("|");
    ImGui::SameLine(); ImGui::Text("Injected: %d", (int)g_injectedList.size());
    if (g_injectedSelectedIdx >= 0 && g_injectedSelectedIdx < (int)g_injectedList.size()) {
        ImGui::SameLine(); ImGui::Text("|");
        ImGui::SameLine(); ImGui::Text("Viewing: %s", g_injectedList[g_injectedSelectedIdx].name.c_str());
    }
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor(11);

    RenderInjectModal();
    RenderEjectModal();
}