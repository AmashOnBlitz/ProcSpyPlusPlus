#include "pch.h"
#include "Render.h"
#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>
#include <tlhelp32.h>
#include <Injector.h>
#include <unordered_map>
#include <PipeServer.h>
#include <future>
#include <mutex>
#include <array>

#define CMDSTRING "#cmd|"

enum class EjectState { Idle, Pending, Done };

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

struct TrackingState {
    bool track = false;
    bool block = false;
};

static const char* g_trackingLabels[] = {
    "Registry Read",
    "Registry Write",
    "File Create"
    "File Read",
    "File Write",
    "Network Send",
    "Network Receive",
    "Thread Create",
    "Memory Alloc",
    "DLL Load",
    "Clipboard Access",
    "Screenshot Capture",
    "Message Box Create",
};
static constexpr int g_trackingCount = (int)(sizeof(g_trackingLabels) / sizeof(g_trackingLabels[0]));

static std::unordered_map<DWORD, std::array<TrackingState, g_trackingCount>> g_trackingStates;;
static std::unordered_map<DWORD, std::vector<std::string>> g_activityLog;
static std::vector<ProcessEntry>  g_processList;
static std::vector<uint8_t>       g_processSelected;
static std::vector<InjectedEntry> g_injectedList;

static std::vector<FailedEntry>   g_injectFailed;
static std::vector<FailedEntry>   g_ejectFailed;

static char g_processFilter[128] = "";
static bool g_showInjectModal = false;
static bool g_showEjectModal = false;
static int  g_ejectSelectedIdx = -1;
static int  g_injectedSelectedIdx = -1;

static std::mutex                 g_ejectResultMutex;
static std::vector<InjectedEntry> g_ejectAllRemaining;
static std::vector<DWORD>         g_ejectAllSucceeded;

static EjectState        g_ejectAllState = EjectState::Idle;
static std::future<void> g_ejectAllFuture;

static EjectState        g_ejectOneState = EjectState::Idle;
static std::future<void> g_ejectOneFuture;

static bool InjectProcess(DWORD pid, const std::string& name) {
    return Injector::Inject(pid, name);
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
        if (InjectProcess(g_processList[i].pid, g_processList[i].name)) {
            g_injectedList.push_back({ g_processList[i].pid, g_processList[i].name });
            g_trackingStates[g_processList[i].pid] = {};
        }
        else {
            g_injectFailed.push_back({ g_processList[i].pid, g_processList[i].name });
        }
    }
}

static bool GhostBtn(const char* label, float w = 0.0f) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.85f, 0.65f, 0.45f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
    bool r = ImGui::Button(label, ImVec2(w, 0));
    ImGui::PopStyleColor(4);
    return r;
}

static void PollEjectAllResult() {
    if (g_ejectAllState != EjectState::Done) return;

    std::lock_guard<std::mutex> lk(g_ejectResultMutex);

    for (DWORD pid : g_ejectAllSucceeded) {
        g_activityLog.erase(pid);
        g_trackingStates.erase(pid);
        auto it = std::find_if(g_injectedList.begin(), g_injectedList.end(),
                               [pid](const InjectedEntry& e) { return e.pid == pid; });
        if (it != g_injectedList.end()) g_injectedList.erase(it);
    }
    g_injectedSelectedIdx = -1;
    g_ejectSelectedIdx = -1;

    g_ejectFailed.clear();
    for (auto& e : g_ejectAllRemaining)
        g_ejectFailed.push_back({ e.pid, e.name });

    g_ejectAllSucceeded.clear();
    g_ejectAllRemaining.clear();
    g_ejectAllState = EjectState::Idle;

    if (g_ejectFailed.empty())
        g_showEjectModal = false;
}

static void PollEjectOneResult() {
    if (g_ejectOneState != EjectState::Done) return;

    std::lock_guard<std::mutex> lk(g_ejectResultMutex);

    for (DWORD pid : g_ejectAllSucceeded) {
        g_activityLog.erase(pid);
        g_trackingStates.erase(pid);
        auto it = std::find_if(g_injectedList.begin(), g_injectedList.end(),
                               [pid](const InjectedEntry& e) { return e.pid == pid; });
        if (it != g_injectedList.end()) {
            int idx = (int)(it - g_injectedList.begin());
            if (g_injectedSelectedIdx == idx)     g_injectedSelectedIdx = -1;
            else if (g_injectedSelectedIdx > idx) g_injectedSelectedIdx--;
            g_injectedList.erase(it);
        }
    }
    g_ejectSelectedIdx = -1;

    g_ejectFailed.clear();
    for (auto& e : g_ejectAllRemaining)
        g_ejectFailed.push_back({ e.pid, e.name });

    g_ejectAllSucceeded.clear();
    g_ejectAllRemaining.clear();
    g_ejectOneState = EjectState::Idle;

    if (g_ejectFailed.empty() && g_injectedList.empty())
        g_showEjectModal = false;
}

static void RenderInjectModal() {
    if (!g_showInjectModal) return;

    ImVec2 display = ImGui::GetIO().DisplaySize;
    float modalW = std::min(500.0f, display.x - 40.0f);
    float baseH = g_injectFailed.empty() ? 420.0f : 490.0f;
    float modalH = std::min(baseH, display.y - 40.0f);

    ImGui::SetNextWindowSize(ImVec2(modalW, modalH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.85f, 0.65f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.18f, 0.85f, 0.65f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));

    ImGui::OpenPopup("##InjectPicker");
    if (ImGui::BeginPopupModal("##InjectPicker", nullptr,
                               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::SetCursorPosX(14);
        ImGui::Text("SELECT TARGET PROCESSES");
        ImGui::PopStyleColor();

        float closeBtnX = ImGui::GetWindowWidth() - 30.0f;
        ImGui::SameLine(closeBtnX);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.2f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        if (ImGui::Button("x##iclose", ImVec2(22, 22))) {
            g_injectFailed.clear();
            g_showInjectModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 2));

        const float fp = ImGui::GetStyle().FramePadding.x;
        const float sp = ImGui::GetStyle().ItemSpacing.x;

        float wRefresh = ImGui::CalcTextSize("Refresh").x + fp * 2.0f;
        float wAll = ImGui::CalcTextSize("All").x + fp * 2.0f;
        float wNone = ImGui::CalcTextSize("None").x + fp * 2.0f;
        float btnTotal = wRefresh + wAll + wNone + sp * 3.0f;
        float availW = ImGui::GetContentRegionAvail().x;
        float filterW = std::max(40.0f, availW - btnTotal);

        ImGui::SetNextItemWidth(filterW);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.90f, 1.0f));
        ImGui::InputTextWithHint("##ifilter", "Filter processes...", g_processFilter, sizeof(g_processFilter));
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (GhostBtn("Refresh", wRefresh)) RefreshProcessList();
        ImGui::SameLine();
        if (GhostBtn("All", wAll)) {
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
        if (GhostBtn("None", wNone))
            for (auto& b : g_processSelected) b = 0;

        ImGui::Dummy(ImVec2(0, 2));

        float lineH = ImGui::GetFrameHeight();
        float itemSp = ImGui::GetStyle().ItemSpacing.y;
        float reserveBottom = lineH + itemSp * 2.0f + 12.0f;
        if (!g_injectFailed.empty())
            reserveBottom += lineH + 56.0f + itemSp * 4.0f + 8.0f;

        float listH = std::max(60.0f, ImGui::GetContentRegionAvail().y - reserveBottom);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
        ImGui::BeginChild("##iproclist", ImVec2(0, listH), true);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.60f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.85f, 0.65f, 0.20f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.18f, 0.85f, 0.65f, 0.30f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.14f, 0.17f, 1.0f));

        std::string filterLow = g_processFilter;
        for (auto& ch : filterLow) ch = (char)tolower(ch);

        for (int i = 0; i < (int)g_processList.size(); i++) {
            const auto& proc = g_processList[i];
            std::string nameLow = proc.name;
            for (auto& ch : nameLow) ch = (char)tolower(ch);
            if (!filterLow.empty() && nameLow.find(filterLow) == std::string::npos) continue;

            bool checked = (g_processSelected[i] != 0);
            const bool wasChecked = checked;

            if (wasChecked)
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

            if (wasChecked) ImGui::PopStyleColor();
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

        char confirmLabel[48];
        snprintf(confirmLabel, sizeof(confirmLabel), "Inject (%d)", selCount);
        float injectBtnW = ImGui::CalcTextSize(confirmLabel).x + fp * 2.0f + 8.0f;
        float injectBtnX = ImGui::GetWindowWidth() - injectBtnW - ImGui::GetStyle().WindowPadding.x;
        ImGui::SameLine(injectBtnX);

        bool canConfirm = (selCount > 0);
        if (!canConfirm) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
        ImGui::PushStyleColor(ImGuiCol_Button, canConfirm ? ImVec4(0.10f, 0.60f, 0.45f, 1.0f) : ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, canConfirm ? ImVec4(0.14f, 0.75f, 0.56f, 1.0f) : ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, canConfirm ? ImVec4(0.08f, 0.50f, 0.38f, 1.0f) : ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        if (ImGui::Button(confirmLabel, ImVec2(injectBtnW, 0)) && canConfirm) {
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

    PollEjectAllResult();
    PollEjectOneResult();

    if (!g_showEjectModal) return;

    ImVec2 display = ImGui::GetIO().DisplaySize;
    float modalW = std::min(420.0f, display.x - 40.0f);
    float baseH = g_ejectFailed.empty() ? 340.0f : 420.0f;
    float modalH = std::min(baseH, display.y - 40.0f);

    ImGui::SetNextWindowSize(ImVec2(modalW, modalH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.70f, 0.20f, 0.20f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.13f, 0.16f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.70f, 0.20f, 0.20f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));

    ImGui::OpenPopup("##EjectPicker");
    if (ImGui::BeginPopupModal("##EjectPicker", nullptr,
                               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.30f, 0.30f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::SetCursorPosX(14);
        ImGui::Text("EJECT");
        ImGui::PopStyleColor();

        float closeBtnX = ImGui::GetWindowWidth() - 30.0f;
        ImGui::SameLine(closeBtnX);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.2f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        if (ImGui::Button("x##eclose", ImVec2(22, 22))) {
            g_ejectFailed.clear();
            g_showEjectModal = false;
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

        const float fp = ImGui::GetStyle().FramePadding.x;
        const float lineH = ImGui::GetFrameHeight();
        const float itemSp = ImGui::GetStyle().ItemSpacing.y;

        float reserveBottom = lineH + itemSp * 2.0f + 14.0f;
        if (!g_ejectFailed.empty())
            reserveBottom += lineH + 56.0f + itemSp * 4.0f + 8.0f;

        float listH = std::max(60.0f, ImGui::GetContentRegionAvail().y - reserveBottom);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
        ImGui::BeginChild("##ejectlist", ImVec2(0, listH), true);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.60f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.70f, 0.15f, 0.15f, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.70f, 0.15f, 0.15f, 0.15f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.70f, 0.15f, 0.15f, 0.35f));

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

        bool anyBusy = (g_ejectAllState == EjectState::Pending || g_ejectOneState == EjectState::Pending);
        bool hasInjected = !g_injectedList.empty();
        bool hasSelected = (g_ejectSelectedIdx >= 0 && g_ejectSelectedIdx < (int)g_injectedList.size());

        auto RedBtn = [](const char* lbl, float w, bool solid) -> bool {
            if (solid) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 0.90f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.10f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.12f, 0.12f, 0.35f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.12f, 0.12f, 0.55f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.30f, 0.30f, 1.0f));
            }
            bool r = ImGui::Button(lbl, ImVec2(w, 0));
            ImGui::PopStyleColor(4);
            return r;
        };

        float wEjectSel = ImGui::CalcTextSize("Eject Selected").x + fp * 2.0f + 8.0f;
        float wEjectAll = ImGui::CalcTextSize("Eject All").x + fp * 2.0f + 8.0f;
        float wPending = ImGui::CalcTextSize("Ejecting...").x + fp * 2.0f + 8.0f;

        bool canSel = hasSelected && !anyBusy;
        bool canAll = hasInjected && !anyBusy;

        if (!canSel) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.40f);
        const char* selLabel = (g_ejectOneState == EjectState::Pending) ? "Ejecting..." : "Eject Selected";
        float       selW = (g_ejectOneState == EjectState::Pending) ? wPending : wEjectSel;
        if (RedBtn(selLabel, selW, canSel) && canSel) {
            InjectedEntry target = g_injectedList[g_ejectSelectedIdx];
            g_ejectOneState = EjectState::Pending;
            g_ejectFailed.clear();
            g_ejectAllSucceeded.clear();
            g_ejectAllRemaining.clear();

            g_ejectOneFuture = std::async(std::launch::async, [target]() {
                bool ok = Injector::Deactivate(target.pid, target.name);
                std::lock_guard<std::mutex> lk(g_ejectResultMutex);
                if (ok) g_ejectAllSucceeded.push_back(target.pid);
                else    g_ejectAllRemaining.push_back(target);
                g_ejectOneState = EjectState::Done;
            });
        }
        if (!canSel) ImGui::PopStyleVar();

        ImGui::SameLine(0, 8);

        if (!canAll) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.40f);
        const char* allLabel = (g_ejectAllState == EjectState::Pending) ? "Ejecting..." : "Eject All";
        float       allW = (g_ejectAllState == EjectState::Pending) ? wPending : wEjectAll;
        if (RedBtn(allLabel, allW, canAll) && canAll) {
            std::vector<InjectedEntry> toEject = g_injectedList;
            g_ejectAllState = EjectState::Pending;
            g_ejectFailed.clear();
            g_ejectAllSucceeded.clear();
            g_ejectAllRemaining.clear();

            g_ejectAllFuture = std::async(std::launch::async, [toEject]() mutable {
                std::vector<InjectedEntry> remaining;
                std::vector<DWORD>         succeeded;
                for (auto& inj : toEject) {
                    if (Injector::Deactivate(inj.pid, inj.name))
                        succeeded.push_back(inj.pid);
                    else
                        remaining.push_back(inj);
                }
                std::lock_guard<std::mutex> lk(g_ejectResultMutex);
                g_ejectAllSucceeded = std::move(succeeded);
                g_ejectAllRemaining = std::move(remaining);
                g_ejectAllState = EjectState::Done;
            });
        }
        if (!canAll) ImGui::PopStyleVar();

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);
}

static void RenderMenuBar() {
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.60f, 0.45f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.75f, 0.56f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.50f, 0.38f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 1.0f, 0.98f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));

    if (ImGui::BeginMenuBar()) {
        bool hasInjected = !g_injectedList.empty();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.60f, 0.45f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.75f, 0.56f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.50f, 0.38f, 1.0f));
        if (ImGui::Button("Inject")) {
            RefreshProcessList();
            g_injectFailed.clear();
            g_showInjectModal = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.10f, 0.10f, 1.0f));
        if (!hasInjected) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
        if (ImGui::Button("Eject") && hasInjected) {
            g_ejectFailed.clear();
            g_showEjectModal = true;
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

static void RenderTrackingPanel(float width, float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
    ImGui::BeginChild("TrackingPanel", ImVec2(width, height), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.7f));
    ImGui::Dummy(ImVec2(0, 2));
    ImGui::Text("  TRACKING (T --> Tracking, B --> Block)");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 3));

    float availW = ImGui::GetContentRegionAvail().x;
    float colW = availW * 0.5f;
    float cbSpacing = ImGui::GetStyle().ItemSpacing.x;

    DWORD          pid = 0;
    TrackingState* states = nullptr;
    bool           hasState = false;

    if (g_injectedSelectedIdx >= 0 && g_injectedSelectedIdx < (int)g_injectedList.size()) {
        pid = g_injectedList[g_injectedSelectedIdx].pid;
        auto it = g_trackingStates.find(pid);
        if (it != g_trackingStates.end()) {
            states = it->second.data();
            hasState = true;
        }
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.14f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.18f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));

    for (int i = 0; i < g_trackingCount; i++) {
        int col = i % 2;

        if (!hasState) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);

        if (col == 1)
            ImGui::SameLine(colW + 4.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.78f, 0.84f, 1.0f));
        ImGui::Text("  %s", g_trackingLabels[i]);
        ImGui::PopStyleColor();

        ImGui::SameLine(col == 0 ? colW - 84.0f : colW + colW - 84.0f);

        bool trackVal = hasState ? states[i].track : false;
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, trackVal
                              ? ImVec4(0.18f, 0.85f, 0.65f, 1.0f)
                              : ImVec4(0.40f, 0.44f, 0.50f, 1.0f));
        char trackId[32];
        snprintf(trackId, sizeof(trackId), "T##t%d", i);
        if (ImGui::Checkbox(trackId, &trackVal) && hasState) {
            states[i].track = trackVal;
            std::string cmd = CMDSTRING;
            cmd += trackVal ? "Track|" : "NoTrack|";
            cmd += g_trackingLabels[i];
            PipeServer::SendCommand(pid, cmd);
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0, cbSpacing);

        bool blockVal = hasState ? states[i].block : false;
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.85f, 0.28f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.30f, 0.10f, 0.10f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.40f, 0.12f, 0.12f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_Text, blockVal
                              ? ImVec4(0.90f, 0.32f, 0.32f, 1.0f)
                              : ImVec4(0.40f, 0.44f, 0.50f, 1.0f));
        char blockId[32];
        snprintf(blockId, sizeof(blockId), "B##b%d", i);
        if (ImGui::Checkbox(blockId, &blockVal) && hasState) {
            states[i].block = blockVal;
            std::string cmd = CMDSTRING;
            cmd += blockVal ? "Block|" : "NoBlock|";
            cmd += g_trackingLabels[i];
            PipeServer::SendCommand(pid, cmd);
        }
        ImGui::PopStyleColor(4);

        if (!hasState) ImGui::PopStyleVar();

        if (col == 1 || i == g_trackingCount - 1)
            ImGui::Dummy(ImVec2(0, 1));
    }

    ImGui::PopStyleColor(3);
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void Render(ImGuiWindowFlags flags) {
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.22f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.08f, 0.09f, 0.11f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.18f, 0.85f, 0.65f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.65f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.85f, 0.65f, 0.18f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.10f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.18f, 0.85f, 0.65f, 0.28f));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.18f, 0.22f, 0.28f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.86f, 0.90f, 1.0f));

    for (auto& inj : g_injectedList) {
        auto newMsgs = PipeServer::DrainMessages(inj.pid);
        if (!newMsgs.empty()) {
            auto& log = g_activityLog[inj.pid];
            for (auto& m : newMsgs)
                log.push_back(m);
            if (log.size() > 5000)
                log.erase(log.begin(), log.begin() + (log.size() - 500));
        }
    }

    ImGui::Begin("FullWindow", nullptr, flags);
    RenderMenuBar();

    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    float  statusBarH = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y + 2.0f;
    float  panelH = windowSize.y - statusBarH - ImGui::GetStyle().ItemSpacing.y;
    float  listWidth = windowSize.x * 0.35f;
    float  rightWidth = windowSize.x - listWidth - ImGui::GetStyle().ItemSpacing.x;

    float trackingH = panelH * 0.40f;
    float activityH = panelH - trackingH - ImGui::GetStyle().ItemSpacing.y;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

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
        const bool wasSel = sel;
        if (wasSel) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));

        char rowLabel[256];
        snprintf(rowLabel, sizeof(rowLabel), "  %-26s  %lu",
                 g_injectedList[i].name.c_str(), (unsigned long)g_injectedList[i].pid);
        if (ImGui::Selectable(rowLabel, sel))
            g_injectedSelectedIdx = (g_injectedSelectedIdx == i) ? -1 : i;

        if (wasSel) ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::SameLine(0, 8);

    ImGui::BeginGroup();

    ImGui::BeginChild("ActivityPanel", ImVec2(rightWidth, activityH), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.7f));
    ImGui::Dummy(ImVec2(0, 2));
    if (g_injectedSelectedIdx >= 0 && g_injectedSelectedIdx < (int)g_injectedList.size()) {
        ImGui::Text("  ACTIVITY  -  %s  (PID %lu)",
                    g_injectedList[g_injectedSelectedIdx].name.c_str(),
                    (unsigned long)g_injectedList[g_injectedSelectedIdx].pid);
    }
    else {
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
    }
    else {
        DWORD viewPid = g_injectedList[g_injectedSelectedIdx].pid;
        auto& log = g_activityLog[viewPid];

        float logH = ImGui::GetContentRegionAvail().y;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.08f, 0.10f, 1.0f));
        ImGui::BeginChild("##actlog", ImVec2(0, logH), false);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.82f, 0.88f, 1.0f));
        for (auto& msg : log) {
            if (msg.rfind("[system]", 0) == 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.85f));
                ImGui::TextUnformatted(msg.c_str());
                ImGui::PopStyleColor();
            }
            else {
                ImGui::TextUnformatted(msg.c_str());
            }
        }
        ImGui::PopStyleColor();

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f)
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    RenderTrackingPanel(rightWidth, trackingH);

    ImGui::EndGroup();

    ImGui::PopStyleVar(2);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
    ImGui::BeginChild("StatusBar", ImVec2(0, statusBarH), false);
    ImGui::Dummy(ImVec2(0, 1));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.40f, 0.45f, 1.0f));

    bool anyBusy = (g_ejectAllState == EjectState::Pending || g_ejectOneState == EjectState::Pending);
    ImGui::Text("  Status: %s", anyBusy ? "Ejecting..." : "Ready");
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