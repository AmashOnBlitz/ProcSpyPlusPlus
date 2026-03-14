#include "pch.h"
#include "Render.h"
#include <windows.h>
#include <gdiplus.h>
#include <tlhelp32.h>
#include <Injector.h>
#include <PipeServer.h>
#include <future>
#include <mutex>
#include <array>
#include <cmath>
#include <memory>
#include <set>
#include <commdlg.h>
#define CMDSTRING "#cmd|"
#include "../Resources/resource.h"

static constexpr ImTextureID k_nullTex = (ImTextureID)0;
static ULONG_PTR g_gdiplusToken = 0;

static ImTextureID UploadRGBATexture(const uint8_t* rgba, int w, int h) {
    GLuint tex = 0;
    glGenTextures(1, &tex);
    if (!tex) return k_nullTex;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glBindTexture(GL_TEXTURE_2D, 0);
    return (ImTextureID)(ImU64)(uintptr_t)tex;
}

static void FreeRGBATexture(ImTextureID tex) {
    if (tex == k_nullTex) return;
    GLuint id = (GLuint)(uintptr_t)(ImU64)tex;
    glDeleteTextures(1, &id);
}

static ImTextureID g_injIconTex = k_nullTex;
static int         g_injIconDim = 0;

static bool LoadIconFromResource(int resourceId, int desiredSz,
                                 ImTextureID& outTex, int& outDim) {
    HICON hIcon = (HICON)LoadImage(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCE(resourceId),
        IMAGE_ICON, desiredSz, desiredSz,
        LR_DEFAULTCOLOR);
    if (!hIcon) return false;

    std::unique_ptr<Gdiplus::Bitmap> bmp(Gdiplus::Bitmap::FromHICON(hIcon));
    DestroyIcon(hIcon);

    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) return false;

    int w = (int)bmp->GetWidth();
    int h = (int)bmp->GetHeight();
    if (w <= 0 || h <= 0) return false;

    Gdiplus::BitmapData bd{};
    Gdiplus::Rect rc(0, 0, w, h);
    if (bmp->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd) != Gdiplus::Ok)
        return false;

    std::vector<uint8_t> rgba((size_t)w * h * 4);
    for (int y = 0; y < h; y++) {
        const uint8_t* row = (const uint8_t*)bd.Scan0 + (ptrdiff_t)y * bd.Stride;
        for (int x = 0; x < w; x++) {
            uint8_t a = row[x * 4 + 3];
            int     idx = (y * w + x) * 4;
            rgba[idx + 0] = 255;
            rgba[idx + 1] = 255;
            rgba[idx + 2] = 255;
            rgba[idx + 3] = a;
        }
    }
    bmp->UnlockBits(&bd);

    outTex = UploadRGBATexture(rgba.data(), w, h);
    outDim = w;
    return outTex != k_nullTex;
}

void Render_Init() {
    Gdiplus::GdiplusStartupInput si{};
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &si, nullptr);
    LoadIconFromResource(IDI_DLL_INJECT, 128, g_injIconTex, g_injIconDim);
}

void Render_Shutdown() {
    FreeRGBATexture(g_injIconTex);
    g_injIconTex = k_nullTex;
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

enum class EjectState { Idle, Pending, Done };
struct ProcessEntry { DWORD pid; std::string name; };
struct InjectedEntry { DWORD pid; std::string name; };
struct FailedEntry { DWORD pid; std::string name; };
struct TrackingState { bool track = false; bool block = false; };

static const char* g_trackingLabels[] = {
    "Registry Read",
    "Registry Write",
    "File Create",
    "File Read",
    "File Write",
    "Network Send",
    "Network Receive",
    "Thread Create",
    "DLL Load",
    "Clipboard Access",
    "Screenshot Capture",
    "Generic Message Box Create",
    "Generic Dialog Box Create",
    "Window Create"
};
static constexpr int g_trackingCount =
(int)(sizeof(g_trackingLabels) / sizeof(g_trackingLabels[0]));

struct TrackingCategory { const char* label; int startIdx; int endIdx; };
static const TrackingCategory g_trackingCategories[] = {
    { "REGISTRY",               0,  1 },
    { "FILE",                   2,  4 },
    { "NETWORK",                5,  6 },
    { "THREAD",                 7,  7 },
    { "DLL",                    8,  8 },
    { "CLIPBOARD / SCREENSHOT", 9, 10 },
    { "UI",                    11, 13 },
};
static constexpr int g_categoryCount =
(int)(sizeof(g_trackingCategories) / sizeof(g_trackingCategories[0]));

struct LogEntry {
    std::vector<std::string> lines;
    ImVec4 accent;
    bool   isSystem = false;
    bool   isError = false;
    bool   isBlocked = false;
};

static std::unordered_map<DWORD, std::array<TrackingState, g_trackingCount>> g_trackingStates;
static std::unordered_map<DWORD, std::vector<LogEntry>>  g_activityLog;
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

static std::vector<ProcessEntry> g_customProcList;
static char                      g_customProcFilter[128] = "";
static int                       g_customProcSelectedIdx = -1;

static char g_customDllPath[512] = "";

static bool g_showCustomInjectDlg = false;
static int  g_customInjectResult = 0;
static char g_customInjectMsg[256] = "";
static bool g_procSelErr = false;
static bool g_dllPathErr = false;

static bool FileExists(const char* path) {
    if (!path || path[0] == '\0') return false;
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

static bool IsFullPath(const char* s) {
    if (!s || !s[0]) return false;
    return (s[1] == ':' && (s[2] == '\\' || s[2] == '/')) ||
        (s[0] == '\\' && s[1] == '\\');
}

static bool BrowseForFile(HWND owner, char* outPath, int maxLen,
                          const char* title, const char* filter) {
    OPENFILENAMEA ofn{};
    char buf[512] = {};
    strncpy_s(buf, sizeof(buf), outPath, _TRUNCATE);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = buf;
    ofn.nMaxFile = sizeof(buf);
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        strncpy_s(outPath, maxLen, buf, _TRUNCATE);
        return true;
    }
    return false;
}

static std::vector<std::string> SplitLines(const std::string& s) {
    std::vector<std::string> out;
    size_t start = 0;
    while (start < s.size()) {
        size_t nl = s.find('\n', start);
        if (nl == std::string::npos) nl = s.size();
        std::string ln = s.substr(start, nl - start);
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (!ln.empty()) out.push_back(std::move(ln));
        start = nl + 1;
    }
    return out;
}

static ImVec4 GetLogAccent(const std::string& msg) {
    const bool sys = msg.rfind("[system]", 0) == 0 || msg.rfind("[System]", 0) == 0;
    const bool err = msg.find("[INTERNAL ERROR]") != std::string::npos;
    if (sys) return ImVec4(0.18f, 0.85f, 0.65f, 1.0f);
    if (err) return ImVec4(0.92f, 0.28f, 0.28f, 1.0f);
    if (msg.find("FILE WRITE") != std::string::npos) return ImVec4(0.95f, 0.58f, 0.18f, 1.0f);
    if (msg.find("FILE DELETE") != std::string::npos) return ImVec4(0.90f, 0.25f, 0.25f, 1.0f);
    if (msg.find("FILE CREATE") != std::string::npos) return ImVec4(0.98f, 0.85f, 0.22f, 1.0f);
    if (msg.find("FILE OPEN") != std::string::npos) return ImVec4(0.45f, 0.75f, 0.95f, 1.0f);
    if (msg.find("FILE READ") != std::string::npos) return ImVec4(0.38f, 0.70f, 0.98f, 1.0f);
    if (msg.find("REGISTRY") != std::string::npos) return ImVec4(0.74f, 0.46f, 0.96f, 1.0f);
    if (msg.find("NETWORK") != std::string::npos) return ImVec4(0.20f, 0.80f, 0.96f, 1.0f);
    if (msg.find("MSGBOX") != std::string::npos) return ImVec4(0.96f, 0.50f, 0.74f, 1.0f);
    if (msg.find("DIALOG") != std::string::npos) return ImVec4(0.96f, 0.50f, 0.74f, 1.0f);
    if (msg.find("THREAD") != std::string::npos) return ImVec4(0.40f, 0.88f, 0.70f, 1.0f);
    if (msg.find("HEAP") != std::string::npos) return ImVec4(0.50f, 0.90f, 0.40f, 1.0f);
    if (msg.find("MEMORY") != std::string::npos) return ImVec4(0.50f, 0.90f, 0.40f, 1.0f);
    if (msg.find("DLL") != std::string::npos) return ImVec4(0.96f, 0.70f, 0.28f, 1.0f);
    if (msg.find("CLIPBOARD") != std::string::npos) return ImVec4(0.80f, 0.64f, 0.96f, 1.0f);
    if (msg.find("SCREENSHOT") != std::string::npos) return ImVec4(0.95f, 0.34f, 0.34f, 1.0f);
    return ImVec4(0.52f, 0.60f, 0.70f, 1.0f);
}

static LogEntry MakeLogEntry(const std::string& raw) {
    LogEntry e;
    e.lines = SplitLines(raw);
    e.accent = GetLogAccent(raw);
    e.isSystem = raw.rfind("[system]", 0) == 0 || raw.rfind("[System]", 0) == 0;
    e.isError = raw.find("[INTERNAL ERROR]") != std::string::npos;
    e.isBlocked = !e.lines.empty() && e.lines[0].find("BLOCKED") != std::string::npos;
    return e;
}

static bool InjectProcess(DWORD pid, const std::string& name) {
    return Injector::Inject(pid, name);
}

void CleanupAPIHooks() {
    for (auto& inj : g_injectedList) {
        auto it = g_trackingStates.find(inj.pid);
        if (it == g_trackingStates.end()) continue;
        auto& states = it->second;
        for (int i = 0; i < g_trackingCount; i++) {
            if (states[i].track) {
                states[i].track = false;
                std::string cmd = CMDSTRING; cmd += "NoTrack|"; cmd += g_trackingLabels[i];
                PipeServer::SendCommand(inj.pid, cmd);
            }
            if (states[i].block) {
                states[i].block = false;
                std::string cmd = CMDSTRING; cmd += "NoBlock|"; cmd += g_trackingLabels[i];
                PipeServer::SendCommand(inj.pid, cmd);
            }
        }
    }
}

void EjectAllSync() {
    for (auto& inj : g_injectedList)
        Injector::Deactivate(inj.pid, inj.name);
    g_injectedList.clear();
    g_trackingStates.clear();
    g_activityLog.clear();
    g_injectedSelectedIdx = -1;
}

static void RefreshProcessList() {
    g_processList.clear();
    g_processSelected.clear();
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32 pe{}; pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            ProcessEntry e; e.pid = pe.th32ProcessID;
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

// Refresh the process list used inside the custom-inject dialog.
static void RefreshCustomProcList() {
    g_customProcList.clear();
    g_customProcSelectedIdx = -1;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32 pe{}; pe.dwSize = sizeof(pe);
    if (Process32First(snap, &pe)) {
        do {
            ProcessEntry e; e.pid = pe.th32ProcessID;
            int sz = WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, nullptr, 0, nullptr, nullptr);
            std::string s(sz, 0);
            WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, &s[0], sz, nullptr, nullptr);
            if (!s.empty() && s.back() == '\0') s.pop_back();
            e.name = s;
            g_customProcList.push_back(e);
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
}

static int CountSelected() {
    int n = 0; for (uint8_t b : g_processSelected) if (b) n++; return n;
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
    for (auto& e : g_ejectAllRemaining) g_ejectFailed.push_back({ e.pid, e.name });
    g_ejectAllSucceeded.clear();
    g_ejectAllRemaining.clear();
    g_ejectAllState = EjectState::Idle;
    if (g_ejectFailed.empty()) g_showEjectModal = false;
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
            if (g_injectedSelectedIdx == idx) g_injectedSelectedIdx = -1;
            else if (g_injectedSelectedIdx > idx) g_injectedSelectedIdx--;
            g_injectedList.erase(it);
        }
    }
    g_ejectSelectedIdx = -1;
    g_ejectFailed.clear();
    for (auto& e : g_ejectAllRemaining) g_ejectFailed.push_back({ e.pid, e.name });
    g_ejectAllSucceeded.clear();
    g_ejectAllRemaining.clear();
    g_ejectOneState = EjectState::Idle;
    if (g_ejectFailed.empty() && g_injectedList.empty()) g_showEjectModal = false;
}

static bool ToolbarIconBtn(const char* id, float btnSize, const char* tooltip,
                           ImTextureID tex = k_nullTex) {
    float padX = (ImGui::GetContentRegionAvail().x - btnSize) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padX);
    ImVec2 screenPos = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton(id, ImVec2(btnSize, btnSize));
    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    ImGui::PopID();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float r = 9.0f;
    ImVec2 p0 = screenPos;
    ImVec2 p1 = ImVec2(screenPos.x + btnSize, screenPos.y + btnSize);

    if (active) {
        dl->AddRectFilled(p0, p1, IM_COL32(14, 72, 55, 220), r);
        dl->AddRect(p0, p1, IM_COL32(46, 217, 165, 255), r, 0, 1.8f);
    }
    else if (hovered) {
        dl->AddRectFilled(p0, p1, IM_COL32(18, 85, 65, 80), r);
        dl->AddRect(p0, p1, IM_COL32(46, 217, 165, 130), r, 0, 1.4f);
        float t = (float)ImGui::GetTime();
        float pulse = 0.5f + 0.5f * sinf(t * 4.5f);
        for (int ring = 2; ring >= 0; ring--) {
            float spread = btnSize * (0.14f + ring * 0.08f) * (0.7f + 0.3f * pulse);
            ImU32 ringA = (ImU32)((18 + ring * 6) * (0.55f + 0.45f * pulse));
            dl->AddRectFilled(
                ImVec2(p0.x - spread, p0.y - spread),
                ImVec2(p1.x + spread, p1.y + spread),
                IM_COL32(46, 217, 165, ringA), r + spread);
        }
    }
    else {
        dl->AddRectFilled(p0, p1, IM_COL32(16, 18, 23, 200), r);
        dl->AddRect(p0, p1, IM_COL32(28, 34, 44, 190), r, 0, 1.0f);
    }

    if (tex != k_nullTex) {
        float  margin = btnSize * 0.18f;
        ImVec2 ip0 = ImVec2(p0.x + margin, p0.y + margin);
        ImVec2 ip1 = ImVec2(p1.x - margin, p1.y - margin);
        if (hovered && !active) {
            float t = (float)ImGui::GetTime();
            float pulse = 0.5f + 0.5f * sinf(t * 4.5f);
            float shimSz = margin * 0.5f;
            float shimX = ip0.x + (ip1.x - ip0.x) * (0.3f + 0.4f * (0.5f + 0.5f * sinf(t * 1.8f)));
            dl->AddRectFilled(
                ImVec2(shimX - shimSz, ip0.y), ImVec2(shimX + shimSz, ip1.y),
                IM_COL32(255, 255, 255, (ImU32)(28 * pulse)), 3.0f);
            uint8_t a = (uint8_t)(185 + (uint8_t)(70.0f * pulse));
            dl->AddImage(tex, ip0, ip1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(46, 217, 165, a));
        }
        else if (active) {
            dl->AddImage(tex, ip0, ip1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(46, 217, 165, 255));
        }
        else {
            dl->AddImage(tex, ip0, ip1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(46, 217, 165, 160));
        }
    }

    if (hovered && tooltip && tooltip[0])
        ImGui::SetTooltip("%s", tooltip);
    return clicked;
}

static void RenderToolbar(float width, float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.063f, 0.071f, 0.090f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("##Toolbar", ImVec2(width, height), false);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      winPos = ImGui::GetWindowPos();
    ImVec2      winSize = ImGui::GetWindowSize();

    dl->AddRectFilledMultiColor(
        winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
        IM_COL32(12, 14, 18, 255), IM_COL32(10, 11, 15, 255),
        IM_COL32(10, 11, 15, 255), IM_COL32(12, 14, 18, 255));
    dl->AddLine(
        ImVec2(winPos.x + winSize.x - 1.0f, winPos.y + 6.0f),
        ImVec2(winPos.x + winSize.x - 1.0f, winPos.y + winSize.y - 6.0f),
        IM_COL32(24, 30, 40, 255), 1.0f);

    ImGui::Dummy(ImVec2(0, 10));
    if (ToolbarIconBtn("##tb_custominject", 38.0f, "Custom DLL Inject", g_injIconTex)) {
        g_customInjectResult = 0;
        g_procSelErr = false;
        g_dllPathErr = false;
        g_customInjectMsg[0] = '\0';
        g_customProcFilter[0] = '\0';
        RefreshCustomProcList();
        g_showCustomInjectDlg = true;
    }
    ImGui::Dummy(ImVec2(0, 6));
    {
        float sepY = winPos.y + ImGui::GetCursorPos().y + 2.0f;
        float sepX0 = winPos.x + 8.0f;
        float sepX1 = winPos.x + winSize.x - 8.0f;
        dl->AddLine(ImVec2(sepX0, sepY), ImVec2(sepX1, sepY), IM_COL32(24, 32, 44, 255), 1.0f);
    }
    ImGui::Dummy(ImVec2(0, 8));
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

static void RenderCustomInjectDialog()
{
    if (!g_showCustomInjectDlg) return;

    ImVec2 display = ImGui::GetIO().DisplaySize;
    float  modalW = std::min(560.0f, display.x - 40.0f);
    float  modalH = std::min(430.0f, display.y - 40.0f);

    ImGui::SetNextWindowSize(ImVec2(modalW, modalH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.07f, 0.08f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.88f, 0.66f, 0.45f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.12f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.13f, 0.15f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.18f, 0.23f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.4f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 7.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 7));

    ImGui::OpenPopup("##CustomInjectDlg");
    if (ImGui::BeginPopupModal("##CustomInjectDlg", nullptr,
                               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove))
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2      winPos = ImGui::GetWindowPos();
        ImVec2      winSize = ImGui::GetWindowSize();
        const float fp = ImGui::GetStyle().FramePadding.x;
        const float isp = ImGui::GetStyle().ItemSpacing.x;

        const float kHeaderH = 42.0f;
        const float kHeaderMidY = winPos.y + kHeaderH * 0.5f;

        dl->AddRectFilledMultiColor(
            winPos, ImVec2(winPos.x + winSize.x, winPos.y + kHeaderH),
            IM_COL32(12, 44, 34, 200), IM_COL32(12, 44, 34, 200),
            IM_COL32(8, 10, 13, 0), IM_COL32(8, 10, 13, 0));

        const float kIconHalf = 10.0f;
        const float kIconX = winPos.x + 18.0f;
        if (g_injIconTex != k_nullTex)
            dl->AddImage(g_injIconTex,
                         ImVec2(kIconX, kHeaderMidY - kIconHalf),
                         ImVec2(kIconX + kIconHalf * 2.0f, kHeaderMidY + kIconHalf),
                         ImVec2(0, 0), ImVec2(1, 1), IM_COL32(46, 217, 165, 200));

        const float kTextH = ImGui::GetTextLineHeight();
        const float kTitleX = kIconX + kIconHalf * 2.0f + 8.0f;
        ImGui::SetCursorScreenPos(ImVec2(kTitleX, kHeaderMidY - kTextH * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
        ImGui::TextUnformatted("CUSTOM DLL INJECTION");
        ImGui::PopStyleColor();

        const float kCloseSz = 22.0f;
        ImGui::SetCursorScreenPos(ImVec2(
            winPos.x + winSize.x - kCloseSz - 6.0f,
            kHeaderMidY - kCloseSz * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.2f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        if (ImGui::Button("x##ciclose", ImVec2(kCloseSz, kCloseSz))) {
            g_showCustomInjectDlg = false;
            g_customInjectResult = 0;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);

        ImGui::SetCursorScreenPos(ImVec2(winPos.x, winPos.y + kHeaderH));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0, 4));

        if (g_customInjectResult != 0) {
            bool   ok = (g_customInjectResult == 1);
            ImU32  bgCol = ok ? IM_COL32(14, 72, 50, 220) : IM_COL32(72, 14, 14, 220);
            ImU32  bordCol = ok ? IM_COL32(46, 217, 165, 140) : IM_COL32(217, 60, 60, 140);
            ImVec4 txCol = ok ? ImVec4(0.18f, 0.85f, 0.65f, 1.0f)
                : ImVec4(0.90f, 0.35f, 0.35f, 1.0f);

            char fullMsg[320];
            snprintf(fullMsg, sizeof(fullMsg), "  %s  %s",
                     ok ? "\xE2\x9C\x93" : "\xE2\x9C\x97", g_customInjectMsg);

            float  bW = ImGui::GetContentRegionAvail().x;
            float  bH = ImGui::GetFrameHeight() + 8.0f;
            ImVec2 bPos = ImGui::GetCursorScreenPos();

            dl->AddRectFilled(bPos, ImVec2(bPos.x + bW, bPos.y + bH), bgCol, 4.0f);
            dl->AddRect(bPos, ImVec2(bPos.x + bW, bPos.y + bH), bordCol, 4.0f, 0, 1.0f);

            ImGui::SetCursorScreenPos(ImVec2(bPos.x + 10.0f,
                                             bPos.y + (bH - ImGui::GetTextLineHeight()) * 0.5f));
            ImGui::PushStyleColor(ImGuiCol_Text, txCol);
            ImGui::TextUnformatted(fullMsg);
            ImGui::PopStyleColor();

            ImGui::SetCursorScreenPos(ImVec2(bPos.x, bPos.y + bH));
            ImGui::Dummy(ImVec2(bW, 0.0f));
            ImGui::Dummy(ImVec2(0, 5));
        }

        ImGui::SetCursorPosX(12.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.56f, 0.66f, 1.0f));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("TARGET PROCESS");
        ImGui::PopStyleColor();
        ImGui::SameLine();

        float refreshW = ImGui::CalcTextSize("Refresh").x + fp * 2.0f + 4.0f;
        float filterW = ImGui::GetContentRegionAvail().x - refreshW - isp - 2.0f;
        ImGui::SetNextItemWidth(filterW);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.86f, 0.90f, 1.0f));
        if (ImGui::InputTextWithHint("##cpfilter", "Filter\xe2\x80\xa6",
                                     g_customProcFilter, sizeof(g_customProcFilter))) {
            g_customProcSelectedIdx = -1;
            g_procSelErr = false;
            g_customInjectResult = 0;
        }
        ImGui::PopStyleColor();
        ImGui::SameLine(0, isp);
        if (GhostBtn("Refresh", refreshW)) RefreshCustomProcList();

        const float lineH = ImGui::GetTextLineHeightWithSpacing();
        const float inputH = ImGui::GetFrameHeight();
        const float btnRowH = inputH + 10.0f;
        const float gaps = ImGui::GetStyle().ItemSpacing.y * 8.0f + 18.0f;
        float listH = std::max(60.0f,
                               ImGui::GetContentRegionAvail().y
                               - lineH - lineH - (inputH + 2.0f) - lineH - btnRowH - gaps);

        if (g_procSelErr) {
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.28f, 0.28f, 0.9f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.2f);
        }
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.85f, 0.65f, 0.20f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.18f, 0.85f, 0.65f, 0.30f));
        ImGui::BeginChild("##cproclist", ImVec2(0, listH), true);

        {
            std::string filterLow = g_customProcFilter;
            for (auto& ch : filterLow) ch = (char)tolower(ch);

            if (g_customProcList.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.38f, 0.42f, 1.0f));
                ImGui::Dummy(ImVec2(0, 4));
                ImGui::Text("  No processes found \xe2\x80\x94 click Refresh");
                ImGui::PopStyleColor();
            }

            for (int i = 0; i < (int)g_customProcList.size(); i++) {
                const auto& proc = g_customProcList[i];
                if (!filterLow.empty()) {
                    std::string nl = proc.name;
                    for (auto& ch : nl) ch = (char)tolower(ch);
                    if (nl.find(filterLow) == std::string::npos) continue;
                }
                bool sel = (g_customProcSelectedIdx == i);
                ImGui::PushStyleColor(ImGuiCol_Text,
                                      sel ? ImVec4(0.18f, 0.85f, 0.65f, 1.0f)
                                      : ImVec4(0.72f, 0.78f, 0.86f, 1.0f));
                char rowLabel[256];
                snprintf(rowLabel, sizeof(rowLabel), "  %-38s PID: %lu",
                         proc.name.c_str(), (unsigned long)proc.pid);
                if (ImGui::Selectable(rowLabel, sel, ImGuiSelectableFlags_SpanAllColumns)) {
                    g_customProcSelectedIdx = (g_customProcSelectedIdx == i) ? -1 : i;
                    g_procSelErr = false;
                    g_customInjectResult = 0;
                }
                ImGui::PopStyleColor();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor(4);

        if (g_procSelErr) {
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.32f, 0.32f, 0.90f));
            ImGui::Text("  \xe2\x9c\x97  Select a process from the list above");
            ImGui::PopStyleColor();
        }
        else {
            ImGui::Dummy(ImVec2(0, lineH));
        }

        if (g_customProcSelectedIdx >= 0 &&
            g_customProcSelectedIdx < (int)g_customProcList.size())
        {
            const auto& sel = g_customProcList[g_customProcSelectedIdx];
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.80f));
            ImGui::Text("  \xe2\x86\x92  %s  (PID %lu)",
                        sel.name.c_str(), (unsigned long)sel.pid);
            ImGui::PopStyleColor();
        }
        else {
            ImGui::Dummy(ImVec2(0, lineH));
        }

        {
            const float labelW = 76.0f;
            const float browseW = ImGui::CalcTextSize("...").x + fp * 2.0f + 10.0f;

            ImGui::SetCursorPosX(12.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.56f, 0.66f, 1.0f));
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("DLL PATH");
            ImGui::PopStyleColor();

            ImGui::SameLine(labelW + 12.0f);
            float dllInputW = ImGui::GetContentRegionAvail().x - browseW - isp;

            if (g_dllPathErr) {
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.85f, 0.28f, 0.28f, 0.9f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.2f);
            }
            ImGui::SetNextItemWidth(dllInputW);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.86f, 0.90f, 1.0f));
            if (ImGui::InputTextWithHint("##dllpath",
                                         "Full path to payload DLL\xe2\x80\xa6",
                                         g_customDllPath, sizeof(g_customDllPath))) {
                g_dllPathErr = false;
                g_customInjectResult = 0;
            }
            ImGui::PopStyleColor();
            if (g_dllPathErr) { ImGui::PopStyleVar(); ImGui::PopStyleColor(); }

            ImGui::SameLine(0, isp);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.14f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.85f, 0.65f, 0.22f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.85f, 0.65f, 0.40f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.65f, 0.75f, 1.0f));
            if (ImGui::Button("...##dllbrowse", ImVec2(browseW, 0))) {
                if (BrowseForFile(nullptr, g_customDllPath, sizeof(g_customDllPath),
                                  "Select Payload DLL",
                                  "DLL Files\0*.dll\0All Files\0*.*\0")) {
                    g_dllPathErr = false;
                    g_customInjectResult = 0;
                }
            }
            ImGui::PopStyleColor(4);

            if (g_dllPathErr) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.32f, 0.32f, 0.90f));
                ImGui::Text("  \xe2\x9c\x97  File not found or path is not absolute");
                ImGui::PopStyleColor();
            }
            else {
                ImGui::Dummy(ImVec2(0, lineH));
            }
        }

        ImGui::Dummy(ImVec2(0, 4));
        {
            float cancelW = ImGui::CalcTextSize("Cancel").x + fp * 2.0f + 14.0f;
            float injectW = ImGui::CalcTextSize("Inject").x + fp * 2.0f + 24.0f;
            float btnStartX = winSize.x - (cancelW + injectW + 8.0f)
                - ImGui::GetStyle().WindowPadding.x;
            ImGui::SetCursorPosX(btnStartX);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.14f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.17f, 0.20f, 0.26f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.16f, 0.21f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.52f, 0.60f, 0.70f, 1.0f));
            if (ImGui::Button("Cancel##ci", ImVec2(cancelW, 0))) {
                g_showCustomInjectDlg = false;
                g_customInjectResult = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(4);

            ImGui::SameLine(0, 8);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.60f, 0.45f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.75f, 0.56f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.50f, 0.38f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            if (ImGui::Button("Inject##ci", ImVec2(injectW, 0))) {
                bool procOk = (g_customProcSelectedIdx >= 0 &&
                               g_customProcSelectedIdx < (int)g_customProcList.size());
                bool dllOk = FileExists(g_customDllPath) && IsFullPath(g_customDllPath);
                g_procSelErr = !procOk;
                g_dllPathErr = !dllOk;
                if (procOk && dllOk) {
                    const auto& target = g_customProcList[g_customProcSelectedIdx];
                    bool ok = Injector::LoadCustomDll(target.pid, target.name.c_str(), g_customDllPath);
                    if (ok) {
                        g_customInjectResult = 1;
                        snprintf(g_customInjectMsg, sizeof(g_customInjectMsg),
                                 "Injected into %s (PID %lu)",
                                 target.name.c_str(), (unsigned long)target.pid);
                    }
                    else {
                        g_customInjectResult = 2;
                        snprintf(g_customInjectMsg, sizeof(g_customInjectMsg),
                                 "LoadCustomDll returned false");
                    }
                }
            }
            ImGui::PopStyleColor(4);
        }

        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(5);
}

static void RenderInjectModal() {
    if (!g_showInjectModal) return;
    ImVec2 display = ImGui::GetIO().DisplaySize;
    float  modalW = std::min(500.0f, display.x - 40.0f);
    float  baseH = g_injectFailed.empty() ? 420.0f : 490.0f;
    float  modalH = std::min(baseH, display.y - 40.0f);
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
                               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4)); ImGui::SetCursorPosX(14);
        ImGui::Text("SELECT TARGET PROCESSES");
        ImGui::PopStyleColor();

        float closeBtnX = ImGui::GetWindowWidth() - 30.0f;
        ImGui::SameLine(closeBtnX);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.2f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        if (ImGui::Button("x##iclose", ImVec2(22, 22)))
        {
            g_injectFailed.clear(); g_showInjectModal = false; ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::Separator(); ImGui::Dummy(ImVec2(0, 2));

        const float fp = ImGui::GetStyle().FramePadding.x;
        const float sp = ImGui::GetStyle().ItemSpacing.x;
        float wRefresh = ImGui::CalcTextSize("Refresh").x + fp * 2.0f;
        float wAll = ImGui::CalcTextSize("All").x + fp * 2.0f;
        float wNone = ImGui::CalcTextSize("None").x + fp * 2.0f;
        float filterW = std::max(40.0f, ImGui::GetContentRegionAvail().x - (wRefresh + wAll + wNone + sp * 3.0f));
        ImGui::SetNextItemWidth(filterW);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.88f, 0.90f, 1.0f));
        ImGui::InputTextWithHint("##ifilter", "Filter processes...", g_processFilter, sizeof(g_processFilter));
        ImGui::PopStyleColor();
        ImGui::SameLine(); if (GhostBtn("Refresh", wRefresh)) RefreshProcessList();
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
        if (GhostBtn("None", wNone)) for (auto& b : g_processSelected) b = 0;
        ImGui::Dummy(ImVec2(0, 2));

        float lineH = ImGui::GetFrameHeight();
        float itemSp = ImGui::GetStyle().ItemSpacing.y;
        float reserveBottom = lineH + itemSp * 2.0f + 12.0f;
        if (!g_injectFailed.empty()) reserveBottom += lineH + 56.0f + itemSp * 4.0f + 8.0f;
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
            bool wasChecked = checked;
            if (wasChecked) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
            char cbId[32]; snprintf(cbId, sizeof(cbId), "##cb%d", i);
            if (ImGui::Checkbox(cbId, &checked)) g_processSelected[i] = checked ? 1 : 0;
            ImGui::SameLine();
            char rowLabel[256];
            snprintf(rowLabel, sizeof(rowLabel), "%-38s PID: %lu",
                     proc.name.c_str(), (unsigned long)proc.pid);
            if (ImGui::Selectable(rowLabel, checked, ImGuiSelectableFlags_SpanAllColumns))
                g_processSelected[i] = checked ? 0 : 1;
            if (wasChecked) ImGui::PopStyleColor();
        }
        ImGui::PopStyleColor(6); ImGui::EndChild(); ImGui::PopStyleColor();

        if (!g_injectFailed.empty()) {
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.35f, 0.35f, 0.9f));
            ImGui::Text("  Failed to inject (%d):", (int)g_injectFailed.size());
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.08f, 0.08f, 1.0f));
            ImGui::BeginChild("##ifailed", ImVec2(0, 56), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.30f, 0.30f, 1.0f));
            for (auto& f : g_injectFailed) {
                char fl[256]; snprintf(fl, sizeof(fl), "  %-38s PID: %lu",
                                       f.name.c_str(), (unsigned long)f.pid);
                ImGui::TextUnformatted(fl);
            }
            ImGui::PopStyleColor(); ImGui::EndChild(); ImGui::PopStyleColor();
        }

        ImGui::Dummy(ImVec2(0, 4));
        int selCount = CountSelected();
        ImGui::PushStyleColor(ImGuiCol_Text, selCount > 0
                              ? ImVec4(0.18f, 0.85f, 0.65f, 0.9f) : ImVec4(0.35f, 0.38f, 0.42f, 1.0f));
        ImGui::Text("  %d process%s selected", selCount, selCount == 1 ? "" : "es");
        ImGui::PopStyleColor();

        char confirmLabel[48]; snprintf(confirmLabel, sizeof(confirmLabel), "Inject (%d)", selCount);
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
            if (g_injectFailed.empty()) { g_showInjectModal = false; ImGui::CloseCurrentPopup(); }
        }
        ImGui::PopStyleColor(4);
        if (!canConfirm) ImGui::PopStyleVar();
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(3); ImGui::PopStyleColor(5);
}

static void RenderEjectModal() {
    if (!g_showEjectModal) return;
    PollEjectAllResult(); PollEjectOneResult();
    if (!g_showEjectModal) return;

    ImVec2 display = ImGui::GetIO().DisplaySize;
    float  modalW = std::min(420.0f, display.x - 40.0f);
    float  baseH = g_ejectFailed.empty() ? 340.0f : 420.0f;
    float  modalH = std::min(baseH, display.y - 40.0f);
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
                               ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.30f, 0.30f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4)); ImGui::SetCursorPosX(14);
        ImGui::Text("EJECT");
        ImGui::PopStyleColor();

        float closeBtnX = ImGui::GetWindowWidth() - 30.0f;
        ImGui::SameLine(closeBtnX);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.2f, 0.2f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.85f, 0.2f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        if (ImGui::Button("x##eclose", ImVec2(22, 22)))
        {
            g_ejectFailed.clear(); g_showEjectModal = false; g_ejectSelectedIdx = -1; ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(4);
        ImGui::Separator(); ImGui::Dummy(ImVec2(0, 4));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.60f, 0.65f, 1.0f));
        ImGui::TextWrapped("Select a process to eject individually, or eject all at once.");
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, 6));

        const float fp = ImGui::GetStyle().FramePadding.x;
        const float lineH = ImGui::GetFrameHeight();
        const float itemSp = ImGui::GetStyle().ItemSpacing.y;
        float reserveBottom = lineH + itemSp * 2.0f + 14.0f;
        if (!g_ejectFailed.empty()) reserveBottom += lineH + 56.0f + itemSp * 4.0f + 8.0f;
        float listH = std::max(60.0f, ImGui::GetContentRegionAvail().y - reserveBottom);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
        ImGui::BeginChild("##ejectlist", ImVec2(0, listH), true);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.60f, 0.65f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.70f, 0.15f, 0.15f, 0.25f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.70f, 0.15f, 0.15f, 0.15f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.70f, 0.15f, 0.15f, 0.35f));

        if (g_injectedList.empty()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.38f, 0.42f, 1.0f));
            ImGui::Dummy(ImVec2(0, 4)); ImGui::Text("  No injected processes");
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
        ImGui::PopStyleColor(4); ImGui::EndChild(); ImGui::PopStyleColor();

        if (!g_ejectFailed.empty()) {
            ImGui::Dummy(ImVec2(0, 4));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.35f, 0.35f, 0.9f));
            ImGui::Text("  Failed to eject (%d):", (int)g_ejectFailed.size());
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.08f, 0.08f, 1.0f));
            ImGui::BeginChild("##efailed", ImVec2(0, 56), true);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.30f, 0.30f, 1.0f));
            for (auto& f : g_ejectFailed) {
                char fl[256]; snprintf(fl, sizeof(fl), "  %-34s PID: %lu",
                                       f.name.c_str(), (unsigned long)f.pid);
                ImGui::TextUnformatted(fl);
            }
            ImGui::PopStyleColor(); ImGui::EndChild(); ImGui::PopStyleColor();
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
            ImGui::PopStyleColor(4); return r;
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
            g_ejectFailed.clear(); g_ejectAllSucceeded.clear(); g_ejectAllRemaining.clear();
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
            g_ejectFailed.clear(); g_ejectAllSucceeded.clear(); g_ejectAllRemaining.clear();
            g_ejectAllFuture = std::async(std::launch::async, [toEject]() mutable {
                std::vector<InjectedEntry> remaining;
                std::vector<DWORD>         succeeded;
                for (auto& inj : toEject) {
                    if (Injector::Deactivate(inj.pid, inj.name)) succeeded.push_back(inj.pid);
                    else                                          remaining.push_back(inj);
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
    ImGui::PopStyleVar(3); ImGui::PopStyleColor(5);
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
        if (ImGui::Button("Inject"))
        {
            RefreshProcessList(); g_injectFailed.clear(); g_showInjectModal = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.12f, 0.12f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.10f, 0.10f, 1.0f));
        if (!hasInjected) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
        if (ImGui::Button("Eject") && hasInjected)
        {
            g_ejectFailed.clear(); g_showEjectModal = true; g_ejectSelectedIdx = -1;
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
    ImGui::PopStyleVar(3); ImGui::PopStyleColor(5);
}

static void RenderCategoryHeader(const char* label, bool disabled) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float       winX = ImGui::GetWindowPos().x;
    float       winW = ImGui::GetWindowSize().x;
    float       pad = ImGui::GetStyle().WindowPadding.x;
    ImGui::Dummy(ImVec2(0, 3));
    ImGui::PushStyleColor(ImGuiCol_Text, disabled
                          ? ImVec4(0.28f, 0.32f, 0.38f, 1.0f) : ImVec4(0.32f, 0.38f, 0.48f, 1.0f));
    ImGui::SetCursorPosX(pad + 2.0f);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    float lineY = ImGui::GetCursorScreenPos().y + 1.0f;
    ImU32 lineCol = disabled ? IM_COL32(32, 38, 48, 255) : IM_COL32(40, 50, 65, 255);
    dl->AddLine(ImVec2(winX + pad + 2.0f, lineY), ImVec2(winX + winW - pad - 2.0f, lineY), lineCol, 1.0f);
    ImGui::Dummy(ImVec2(0, 3));
}

static void RenderTrackingPanel(float width, float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
    ImGui::BeginChild("TrackingPanel", ImVec2(width, height), true);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.7f));
    ImGui::Dummy(ImVec2(0, 2));
    ImGui::Text("  TRACKING  (T = Track  |  B = Block)");
    ImGui::PopStyleColor();
    ImGui::Separator(); ImGui::Dummy(ImVec2(0, 3));

    float availW = ImGui::GetContentRegionAvail().x;
    float colW = availW * 0.5f;
    float cbSpacing = ImGui::GetStyle().ItemSpacing.x;

    DWORD          pid = 0;
    TrackingState* states = nullptr;
    bool           hasState = false;
    if (g_injectedSelectedIdx >= 0 && g_injectedSelectedIdx < (int)g_injectedList.size()) {
        pid = g_injectedList[g_injectedSelectedIdx].pid;
        auto it = g_trackingStates.find(pid);
        if (it != g_trackingStates.end()) { states = it->second.data(); hasState = true; }
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.14f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.18f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));

    for (int ci = 0; ci < g_categoryCount; ci++) {
        const TrackingCategory& cat = g_trackingCategories[ci];
        RenderCategoryHeader(cat.label, !hasState);
        int itemsInCat = cat.endIdx - cat.startIdx + 1;
        for (int li = 0; li < itemsInCat; li++) {
            int i = cat.startIdx + li;
            int col = li % 2;
            if (!hasState) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
            if (col == 1) ImGui::SameLine(colW + 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.78f, 0.84f, 1.0f));
            ImGui::Text("  %s", g_trackingLabels[i]);
            ImGui::PopStyleColor();
            ImGui::SameLine(col == 0 ? colW - 84.0f : colW + colW - 84.0f);

            bool trackVal = hasState ? states[i].track : false;
            ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, trackVal
                                  ? ImVec4(0.18f, 0.85f, 0.65f, 1.0f) : ImVec4(0.40f, 0.44f, 0.50f, 1.0f));
            char trackId[32]; snprintf(trackId, sizeof(trackId), "T##t%d", i);
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
                                  ? ImVec4(0.90f, 0.32f, 0.32f, 1.0f) : ImVec4(0.40f, 0.44f, 0.50f, 1.0f));
            char blockId[32]; snprintf(blockId, sizeof(blockId), "B##b%d", i);
            if (ImGui::Checkbox(blockId, &blockVal) && hasState) {
                states[i].block = blockVal;
                std::string cmd = CMDSTRING;
                cmd += blockVal ? "Block|" : "NoBlock|";
                cmd += g_trackingLabels[i];
                PipeServer::SendCommand(pid, cmd);
            }
            ImGui::PopStyleColor(4);
            if (!hasState) ImGui::PopStyleVar();
            if (col == 1 || li == itemsInCat - 1) ImGui::Dummy(ImVec2(0, 1));
        }
    }
    ImGui::PopStyleColor(3);
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

static void RenderActivityLog(DWORD pid, float width, float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
    ImGui::BeginChild("ActivityPanel", ImVec2(width, height), true);
    {
        const InjectedEntry* entry =
            (g_injectedSelectedIdx >= 0 && g_injectedSelectedIdx < (int)g_injectedList.size())
            ? &g_injectedList[g_injectedSelectedIdx] : nullptr;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.70f));
        ImGui::Dummy(ImVec2(0, 2));
        if (entry)
            ImGui::Text("  ACTIVITY  \xe2\x80\x94  %s  (PID %lu)",
                        entry->name.c_str(), (unsigned long)entry->pid);
        else
            ImGui::Text("  ACTIVITY");
        ImGui::PopStyleColor();

        auto logIt = g_activityLog.find(pid);
        int  cnt = (logIt != g_activityLog.end()) ? (int)logIt->second.size() : 0;
        if (entry && cnt > 0) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.32f, 0.37f, 0.44f, 1.0f));
            ImGui::Text("  %d events", cnt);
            ImGui::PopStyleColor();
            float clearW = ImGui::CalcTextSize("Clear").x + ImGui::GetStyle().FramePadding.x * 2.0f + 6.0f;
            float rightX = ImGui::GetWindowWidth() - clearW - ImGui::GetStyle().WindowPadding.x - 2.0f;
            ImGui::SameLine(rightX);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.22f, 0.28f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.28f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.38f, 0.44f, 0.52f, 1.0f));
            if (ImGui::SmallButton("Clear") && logIt != g_activityLog.end())
                logIt->second.clear();
            ImGui::PopStyleColor(4);
        }
    }
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 2));

    if (g_injectedSelectedIdx < 0 || g_injectedSelectedIdx >= (int)g_injectedList.size()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.32f, 0.38f, 1.0f));
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::Text("  Select an injected process to view its activity");
        ImGui::PopStyleColor();
        ImGui::EndChild(); ImGui::PopStyleColor(); return;
    }

    auto& log = g_activityLog[pid];
    float logH = ImGui::GetContentRegionAvail().y;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.048f, 0.054f, 0.068f, 1.0f));
    ImGui::BeginChild("##actlog", ImVec2(0, logH), false, ImGuiWindowFlags_HorizontalScrollbar);
    {
        float gs = ImGui::GetIO().FontGlobalScale;
        if (gs > 0.01f) ImGui::SetWindowFontScale(1.0f / gs);
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float       winX = ImGui::GetWindowPos().x;

    if (log.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.25f, 0.30f, 0.36f, 1.0f));
        ImGui::Dummy(ImVec2(0, 10));
        ImGui::Text("  Waiting for activity...");
        ImGui::PopStyleColor();
    }

    bool isFirst = true;
    for (const auto& entry : log) {
        if (entry.lines.empty()) continue;
        if (!isFirst) {
            ImGui::Dummy(ImVec2(0, 4));
            float sy = ImGui::GetCursorScreenPos().y;
            dl->AddLine(ImVec2(winX + 14.0f, sy), ImVec2(winX + ImGui::GetWindowWidth() - 14.0f, sy),
                        IM_COL32(22, 28, 38, 255), 1.0f);
            ImGui::Dummy(ImVec2(0, 4));
        }
        isFirst = false;

        ImU32  accentU = ImGui::ColorConvertFloat4ToU32(entry.accent);
        float  barY0 = ImGui::GetCursorScreenPos().y;
        ImGui::PushStyleColor(ImGuiCol_Text, entry.accent);
        ImGui::Text("  %s", entry.lines[0].c_str());
        ImGui::PopStyleColor();

        if (entry.lines.size() > 1) {
            ImVec4 detailCol = { 0.56f, 0.63f, 0.73f, 1.0f };
            if (entry.isSystem) detailCol = { 0.16f, 0.72f, 0.55f, 0.85f };
            if (entry.isError)  detailCol = { 0.82f, 0.38f, 0.38f, 1.0f };
            ImGui::PushStyleColor(ImGuiCol_Text, detailCol);
            for (size_t li = 1; li < entry.lines.size(); li++)
                ImGui::Text("  %s", entry.lines[li].c_str());
            ImGui::PopStyleColor();
        }
        float barY1 = ImGui::GetCursorScreenPos().y;
        dl->AddRectFilled(ImVec2(winX + 4.0f, barY0), ImVec2(winX + 7.0f, barY1), accentU);
    }

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 6.0f)
        ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::PopStyleColor();
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
            for (auto& m : newMsgs) log.push_back(MakeLogEntry(m));
            if (log.size() > 5000)
                log.erase(log.begin(), log.begin() + (int)(log.size() - 500));
        }
    }
    {
        std::vector<DWORD> dead;
        for (auto& inj : g_injectedList) {
            if (PipeServer::IsDisconnected(inj.pid)) {
                dead.push_back(inj.pid);
                std::string note = "[System] Connection lost \xe2\x80\x94 ";
                note += inj.name; note += " (PID "; note += std::to_string(inj.pid); note += ") has exited";
                g_activityLog[inj.pid].push_back(MakeLogEntry(note));
            }
        }
        for (DWORD pid : dead) {
            PipeServer::Cleanup(pid);
            g_trackingStates.erase(pid);
            auto it = std::find_if(g_injectedList.begin(), g_injectedList.end(),
                                   [pid](const InjectedEntry& e) { return e.pid == pid; });
            if (it != g_injectedList.end()) {
                int idx = (int)(it - g_injectedList.begin());
                if (g_injectedSelectedIdx == idx) g_injectedSelectedIdx = -1;
                else if (g_injectedSelectedIdx > idx) g_injectedSelectedIdx--;
                g_injectedList.erase(it);
            }
        }
    }

    ImGui::Begin("FullWindow", nullptr, flags);
    RenderMenuBar();

    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    float  statusBarH = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y + 2.0f;
    float  panelH = windowSize.y - statusBarH - ImGui::GetStyle().ItemSpacing.y;
    float  itemSp = ImGui::GetStyle().ItemSpacing.x;
    const float toolbarW = 52.0f;
    float  avail = windowSize.x - toolbarW - itemSp;
    float  listWidth = avail * 0.22f;
    float  rightWidth = avail - listWidth - itemSp;
    float  trackingH = panelH * 0.28f;
    float  activityH = panelH - trackingH - ImGui::GetStyle().ItemSpacing.y;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

    RenderToolbar(toolbarW, panelH);
    ImGui::SameLine(0, itemSp);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.11f, 0.14f, 1.0f));
    ImGui::BeginChild("InjectedList", ImVec2(listWidth, panelH), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 0.7f));
    ImGui::Dummy(ImVec2(0, 2)); ImGui::Text("  INJECTED PROCESSES");
    ImGui::PopStyleColor();
    ImGui::Separator(); ImGui::Dummy(ImVec2(0, 2));

    if (g_injectedList.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.28f, 0.32f, 0.38f, 1.0f));
        ImGui::Dummy(ImVec2(0, 4)); ImGui::Text("  No active injections");
        ImGui::PopStyleColor();
    }
    for (int i = 0; i < (int)g_injectedList.size(); i++) {
        bool sel = (g_injectedSelectedIdx == i);
        if (sel) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.18f, 0.85f, 0.65f, 1.0f));
        char rowLabel[256];
        snprintf(rowLabel, sizeof(rowLabel), "  %-22s  %lu",
                 g_injectedList[i].name.c_str(), (unsigned long)g_injectedList[i].pid);
        if (ImGui::Selectable(rowLabel, sel))
            g_injectedSelectedIdx = (g_injectedSelectedIdx == i) ? -1 : i;
        if (sel) ImGui::PopStyleColor();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 8);
    DWORD viewPid = (g_injectedSelectedIdx >= 0 && g_injectedSelectedIdx < (int)g_injectedList.size())
        ? g_injectedList[g_injectedSelectedIdx].pid : 0;

    ImGui::BeginGroup();
    RenderActivityLog(viewPid, rightWidth, activityH);
    RenderTrackingPanel(rightWidth, trackingH);
    ImGui::EndGroup();

    ImGui::PopStyleVar(2);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.09f, 1.0f));
    ImGui::BeginChild("StatusBar", ImVec2(0, statusBarH), false);
    ImGui::Dummy(ImVec2(0, 1));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.32f, 0.37f, 0.44f, 1.0f));
    bool anyBusy = (g_ejectAllState == EjectState::Pending || g_ejectOneState == EjectState::Pending);
    ImGui::Text("  Status: %s", anyBusy ? "Ejecting..." : "Ready");
    ImGui::SameLine(); ImGui::Text("|");
    ImGui::SameLine(); ImGui::Text("Injected: %d", (int)g_injectedList.size());
    if (g_injectedSelectedIdx >= 0 && g_injectedSelectedIdx < (int)g_injectedList.size()) {
        ImGui::SameLine(); ImGui::Text("|");
        ImGui::SameLine(); ImGui::Text("Viewing: %s",
                                       g_injectedList[g_injectedSelectedIdx].name.c_str());
    }
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor(11);

    RenderInjectModal();
    RenderEjectModal();
    RenderCustomInjectDialog();
}