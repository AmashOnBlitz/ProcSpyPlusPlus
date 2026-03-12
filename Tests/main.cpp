/*
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║           Hook Test Bench  —  Win32 Direct2D Dummy App             ║
 * ║   Inject your hook DLL, then click each button to trigger the API  ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 *
 *  Build (MinGW-w64 / g++):
 *    g++ -o dummy.exe main.cpp ^
 *        -ld2d1 -ldwrite -luser32 -lgdi32 -lole32 -luuid ^
 *        -lcomctl32 -ladvapi32 -lkernel32 -mwindows -std=c++17
 *
 *  For TaskDialog/TaskDialogIndirect you may also need a comctl32 v6
 *  manifest (app.manifest) linked in via windres.
 */

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <winreg.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <functional>
#include <deque>
#include <algorithm>
#include <cstring>
#include <cwchar>
#include <cstdio>

// ─────────────────────────────────────────────────────────────────────────────
//  Colour palette
// ─────────────────────────────────────────────────────────────────────────────
static inline D2D1_COLOR_F C(float r, float g, float b, float a = 1.f)
{ return {r, g, b, a}; }

namespace Pal {
    const D2D1_COLOR_F BG       = C(0.047f, 0.059f, 0.082f);   // #0C0F15
    const D2D1_COLOR_F Header   = C(0.063f, 0.078f, 0.106f);   // #101428
    const D2D1_COLOR_F Panel    = C(0.075f, 0.094f, 0.126f);   // #131820
    const D2D1_COLOR_F PanelHov = C(0.118f, 0.153f, 0.204f);   // hover
    const D2D1_COLOR_F LogBg    = C(0.031f, 0.039f, 0.055f);   // #080A0E
    const D2D1_COLOR_F Divider  = C(0.11f,  0.14f,  0.19f);
    const D2D1_COLOR_F Text     = C(0.843f, 0.882f, 0.929f);
    const D2D1_COLOR_F TextDim  = C(0.380f, 0.455f, 0.545f);
    const D2D1_COLOR_F Accent   = C(0.204f, 0.827f, 0.573f);   // emerald
    // category accent colours
    const D2D1_COLOR_F CatFile  = C(0.220f, 0.631f, 1.000f);   // sky-blue
    const D2D1_COLOR_F CatMsg   = C(0.996f, 0.788f, 0.220f);   // amber
    const D2D1_COLOR_F CatDlg   = C(0.996f, 0.557f, 0.220f);   // orange
    const D2D1_COLOR_F CatMem   = C(0.757f, 0.373f, 1.000f);   // violet
    const D2D1_COLOR_F CatReg   = C(0.996f, 0.357f, 0.357f);   // coral-red
}

// ─────────────────────────────────────────────────────────────────────────────
//  Data structures
// ─────────────────────────────────────────────────────────────────────────────
struct Button {
    std::wstring  label;
    std::wstring  catName;
    D2D1_RECT_F   rect;
    D2D1_COLOR_F  accent;
    std::function<std::wstring()> test;
};

struct LogLine { std::wstring text; D2D1_COLOR_F color; };

// ─────────────────────────────────────────────────────────────────────────────
//  Globals
// ─────────────────────────────────────────────────────────────────────────────
static HINSTANCE              g_hInst  = nullptr;
static ID2D1Factory*          g_pD2D   = nullptr;
static IDWriteFactory*        g_pDW    = nullptr;
static ID2D1HwndRenderTarget* g_pRT    = nullptr;
static ID2D1SolidColorBrush*  g_pBr    = nullptr;
static IDWriteTextFormat*     g_fTitle = nullptr;
static IDWriteTextFormat*     g_fSub   = nullptr;
static IDWriteTextFormat*     g_fCat   = nullptr;
static IDWriteTextFormat*     g_fBtn   = nullptr;
static IDWriteTextFormat*     g_fLog   = nullptr;

static std::vector<Button>    g_btns;
static std::deque<LogLine>    g_log;
static int                    g_hov   = -1;

static constexpr float WIN_W  = 1080.f;
static constexpr float WIN_H  = 760.f;
static constexpr int   LOG_MAX = 60;

// Layout constants (shared between BuildButtons and Render)
static constexpr float HDR_H   = 64.f;
static constexpr float LOG_H   = 160.f;
static constexpr float MARGIN  = 16.f;
static constexpr float LBL_W   = 158.f;   // category label column width
static constexpr float BTN_H   = 34.f;
static constexpr float BTN_GX  = 8.f;     // horizontal gap between buttons
static constexpr float BTN_GY  = 7.f;     // vertical gap between button rows
static constexpr float ROW_SEP = 14.f;    // extra vertical gap between categories

// ─────────────────────────────────────────────────────────────────────────────
//  Small helpers
// ─────────────────────────────────────────────────────────────────────────────
static void AddLog(const std::wstring& msg,
                   D2D1_COLOR_F col = Pal::Accent)
{
    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t ts[24];
    swprintf(ts, 24, L"[%02d:%02d:%02d]  ", st.wHour, st.wMinute, st.wSecond);
    g_log.push_front({std::wstring(ts) + msg, col});
    if ((int)g_log.size() > LOG_MAX) g_log.pop_back();
}

static std::wstring TempFile(const wchar_t* name)
{
    wchar_t buf[MAX_PATH];
    GetTempPathW(MAX_PATH, buf);
    return std::wstring(buf) + name;
}

static std::wstring ErrStr(DWORD e)
{ return L" (err=" + std::to_wstring(e) + L")"; }

// ═════════════════════════════════════════════════════════════════════════════
//  TEST FUNCTIONS
// ═════════════════════════════════════════════════════════════════════════════

// ── File::Creation ────────────────────────────────────────────────────────────
static std::wstring T_CreateFileA()
{
    std::wstring wp = TempFile(L"htb_cfa.bin");
    std::string p(wp.begin(), wp.end());
    HANDLE h = CreateFileA(p.c_str(),
                           GENERIC_READ|GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) { CloseHandle(h); return L"CreateFileA OK  ->  " + wp; }
    return L"CreateFileA FAILED" + ErrStr(GetLastError());
}

static std::wstring T_CreateFileW()
{
    std::wstring wp = TempFile(L"htb_cfw.bin");
    HANDLE h = CreateFileW(wp.c_str(),
                           GENERIC_READ|GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) { CloseHandle(h); return L"CreateFileW OK  ->  " + wp; }
    return L"CreateFileW FAILED" + ErrStr(GetLastError());
}

// ── File::Read ────────────────────────────────────────────────────────────────
static void EnsureReadFile()
{
    std::wstring wp = TempFile(L"htb_read.bin");
    HANDLE h = CreateFileW(wp.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD w; const char d[] = "HOOKTEST_READ_PAYLOAD_1234567890";
        WriteFile(h, d, (DWORD)strlen(d), &w, nullptr);
        CloseHandle(h);
    }
}

static std::wstring T_ReadFile()
{
    EnsureReadFile();
    std::wstring wp = TempFile(L"htb_read.bin");
    HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return L"ReadFile -- open failed" + ErrStr(GetLastError());
    char buf[64] = {}; DWORD nRead = 0;
    BOOL ok = ReadFile(h, buf, (DWORD)sizeof(buf)-1, &nRead, nullptr);
    CloseHandle(h);
    if (ok) return L"ReadFile OK  ->  " + std::to_wstring(nRead) + L" bytes read";
    return L"ReadFile FAILED" + ErrStr(GetLastError());
}

static std::wstring T_ReadFileEx()
{
    EnsureReadFile();
    std::wstring wp = TempFile(L"htb_read.bin");
    HANDLE h = CreateFileW(wp.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return L"ReadFileEx -- open failed" + ErrStr(GetLastError());
    static char rxBuf[64];
    OVERLAPPED ov = {}; ov.hEvent = CreateEventW(nullptr,TRUE,FALSE,nullptr);
    ReadFileEx(h, rxBuf, (DWORD)sizeof(rxBuf)-1, &ov,
               [](DWORD,DWORD,LPOVERLAPPED){});
    WaitForSingleObjectEx(ov.hEvent, 500, TRUE);
    CloseHandle(ov.hEvent); CloseHandle(h);
    return L"ReadFileEx OK  (overlapped, 500 ms wait)";
}

// ── File::Write ───────────────────────────────────────────────────────────────
static std::wstring T_WriteFile()
{
    std::wstring wp = TempFile(L"htb_wf.bin");
    HANDLE h = CreateFileW(wp.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return L"WriteFile -- open failed" + ErrStr(GetLastError());
    const char pl[] = "HOOKTEST_WRITEFILE_PAYLOAD\n";
    DWORD written = 0;
    BOOL ok = WriteFile(h, pl, (DWORD)strlen(pl), &written, nullptr);
    CloseHandle(h);
    if (ok) return L"WriteFile OK  ->  " + std::to_wstring(written) + L" bytes  ->  " + wp;
    return L"WriteFile FAILED" + ErrStr(GetLastError());
}

static std::wstring T_WriteFileEx()
{
    std::wstring wp = TempFile(L"htb_wfex.bin");
    HANDLE h = CreateFileW(wp.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return L"WriteFileEx -- open failed" + ErrStr(GetLastError());
    const char pl[] = "HOOKTEST_WRITEFILEEX_PAYLOAD\n";
    OVERLAPPED ov = {}; ov.hEvent = CreateEventW(nullptr,TRUE,FALSE,nullptr);
    WriteFileEx(h, pl, (DWORD)strlen(pl), &ov,
                [](DWORD,DWORD,LPOVERLAPPED){});
    WaitForSingleObjectEx(ov.hEvent, 500, TRUE);
    CloseHandle(ov.hEvent); CloseHandle(h);
    return L"WriteFileEx OK  ->  " + wp;
}

// ── MsgBox ────────────────────────────────────────────────────────────────────
static std::wstring T_MsgBoxA()
{
    int r = MessageBoxA(nullptr,
        "Hook test: MessageBoxA fired!\nThis dialog was intercepted.",
        "HookTestBench", MB_OK | MB_ICONINFORMATION);
    return L"MessageBoxA returned " + std::to_wstring(r);
}

static std::wstring T_MsgBoxW()
{
    int r = MessageBoxW(nullptr,
        L"Hook test: MessageBoxW fired!\nThis dialog was intercepted.",
        L"HookTestBench", MB_OK | MB_ICONINFORMATION);
    return L"MessageBoxW returned " + std::to_wstring(r);
}

// ── Dialog ────────────────────────────────────────────────────────────────────
// We pass a bogus resource ID (9999); it will fail loading the resource,
// but the hook fires *before* the resource is resolved.
static std::wstring T_DialogBoxParamA()
{
    INT_PTR r = DialogBoxParamA(g_hInst,
                                MAKEINTRESOURCEA(9999), nullptr, nullptr, 0);
    return L"DialogBoxParamA called, ret=" + std::to_wstring(r) +
           L"  (resource 9999 not found -- hook still fires)";
}

static std::wstring T_DialogBoxParamW()
{
    INT_PTR r = DialogBoxParamW(g_hInst,
                                MAKEINTRESOURCEW(9999), nullptr, nullptr, 0);
    return L"DialogBoxParamW called, ret=" + std::to_wstring(r);
}

static std::wstring T_CreateDialogParamA()
{
    HWND h = CreateDialogParamA(g_hInst,
                                MAKEINTRESOURCEA(9999), nullptr, nullptr, 0);
    return std::wstring(L"CreateDialogParamA called, HWND=") +
           (h ? L"valid" : L"NULL (expected -- hook fired)");
}

static std::wstring T_CreateDialogParamW()
{
    HWND h = CreateDialogParamW(g_hInst,
                                MAKEINTRESOURCEW(9999), nullptr, nullptr, 0);
    return std::wstring(L"CreateDialogParamW called, HWND=") +
           (h ? L"valid" : L"NULL (expected -- hook fired)");
}

// TaskDialog / TaskDialogIndirect -- loaded dynamically (comctl32 v6 / manifest)
typedef HRESULT(WINAPI* PFN_TaskDialog)(
    HWND, HINSTANCE, PCWSTR, PCWSTR, PCWSTR,
    TASKDIALOG_COMMON_BUTTON_FLAGS, PCWSTR, int*);

typedef HRESULT(WINAPI* PFN_TaskDialogIndirect)(
    const TASKDIALOGCONFIG*, int*, int*, BOOL*);

static std::wstring T_TaskDialog()
{
    HMODULE hCC = GetModuleHandleW(L"comctl32.dll");
    if (!hCC) hCC = LoadLibraryW(L"comctl32.dll");
    if (!hCC) return L"TaskDialog -- comctl32 not loaded";
    auto fn = (PFN_TaskDialog)GetProcAddress(hCC, "TaskDialog");
    if (!fn) return L"TaskDialog -- GetProcAddress failed (need comctl32 v6 / manifest)";
    int btn = 0;
    HRESULT hr = fn(nullptr, nullptr,
                    L"HookTestBench", L"TaskDialog Hook Test",
                    L"Your hook should intercept this TaskDialog call.",
                    TDCBF_OK_BUTTON, nullptr, &btn);
    wchar_t res[64];
    swprintf(res, 64, L"TaskDialog hr=0x%08X  btn=%d", (unsigned)hr, btn);
    return res;
}

static std::wstring T_TaskDialogIndirect()
{
    HMODULE hCC = GetModuleHandleW(L"comctl32.dll");
    if (!hCC) hCC = LoadLibraryW(L"comctl32.dll");
    if (!hCC) return L"TaskDialogIndirect -- comctl32 not loaded";
    auto fn = (PFN_TaskDialogIndirect)GetProcAddress(hCC, "TaskDialogIndirect");
    if (!fn) return L"TaskDialogIndirect -- GetProcAddress failed";

    TASKDIALOGCONFIG cfg  = {};
    cfg.cbSize             = sizeof(cfg);
    cfg.pszWindowTitle     = L"HookTestBench";
    cfg.pszMainInstruction = L"TaskDialogIndirect Hook Test";
    cfg.pszContent         = L"Your hook should intercept this.";
    cfg.dwCommonButtons    = TDCBF_OK_BUTTON;
    int btn = 0, radio = 0; BOOL verif = FALSE;
    HRESULT hr = fn(&cfg, &btn, &radio, &verif);
    wchar_t res[64];
    swprintf(res, 64, L"TaskDialogIndirect hr=0x%08X  btn=%d", (unsigned)hr, btn);
    return res;
}

// ── Memory::Alloc ─────────────────────────────────────────────────────────────
static std::wstring T_VirtualAlloc()
{
    LPVOID p = VirtualAlloc(nullptr, 4096,
                            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!p) return L"VirtualAlloc FAILED" + ErrStr(GetLastError());
    memset(p, 0xAA, 4096);
    VirtualFree(p, 0, MEM_RELEASE);
    return L"VirtualAlloc OK  ->  4096 bytes committed (PAGE_READWRITE)";
}

static std::wstring T_VirtualAllocEx()
{
    HANDLE hp = OpenProcess(
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, GetCurrentProcessId());
    if (!hp) return L"VirtualAllocEx -- OpenProcess failed" + ErrStr(GetLastError());
    LPVOID p = VirtualAllocEx(hp, nullptr, 8192,
                              MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    std::wstring r = p
        ? L"VirtualAllocEx OK  ->  8192 bytes in self-process"
        : L"VirtualAllocEx FAILED" + ErrStr(GetLastError());
    if (p) VirtualFreeEx(hp, p, 0, MEM_RELEASE);
    CloseHandle(hp);
    return r;
}

static std::wstring T_HeapAlloc()
{
    HANDLE heap = GetProcessHeap();
    LPVOID p = HeapAlloc(heap, HEAP_ZERO_MEMORY, 2048);
    if (!p) return L"HeapAlloc FAILED";
    memset(p, 0xBB, 2048);
    HeapFree(heap, 0, p);
    return L"HeapAlloc OK  ->  2048 bytes (HEAP_ZERO_MEMORY)";
}

static std::wstring T_HeapReAlloc()
{
    HANDLE heap = GetProcessHeap();
    LPVOID p = HeapAlloc(heap, 0, 512);
    if (!p) return L"HeapReAlloc -- initial alloc failed";
    LPVOID p2 = HeapReAlloc(heap, 0, p, 4096);
    if (p2) { HeapFree(heap, 0, p2); return L"HeapReAlloc OK  ->  512 -> 4096 bytes"; }
    HeapFree(heap, 0, p);
    return L"HeapReAlloc FAILED";
}

// ── Memory::Free ──────────────────────────────────────────────────────────────
static std::wstring T_VirtualFree()
{
    LPVOID p = VirtualAlloc(nullptr, 4096, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!p) return L"VirtualFree -- alloc failed";
    BOOL ok = VirtualFree(p, 0, MEM_RELEASE);
    return ok ? L"VirtualFree OK" : L"VirtualFree FAILED" + ErrStr(GetLastError());
}

static std::wstring T_VirtualFreeEx()
{
    HANDLE hp = OpenProcess(
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, GetCurrentProcessId());
    if (!hp) return L"VirtualFreeEx -- OpenProcess failed";
    LPVOID p = VirtualAllocEx(hp, nullptr, 4096, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!p) { CloseHandle(hp); return L"VirtualFreeEx -- alloc failed"; }
    BOOL ok = VirtualFreeEx(hp, p, 0, MEM_RELEASE);
    CloseHandle(hp);
    return ok ? L"VirtualFreeEx OK" : L"VirtualFreeEx FAILED";
}

static std::wstring T_HeapFree()
{
    HANDLE heap = GetProcessHeap();
    LPVOID p = HeapAlloc(heap, 0, 256);
    if (!p) return L"HeapFree -- alloc failed";
    BOOL ok = HeapFree(heap, 0, p);
    return ok ? L"HeapFree OK  ->  256 bytes returned to heap"
              : L"HeapFree FAILED";
}

// ── Registry::Read ────────────────────────────────────────────────────────────
static void EnsureRegKey()
{
    HKEY hk = nullptr;
    RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\HookTestBench",
                    0, nullptr, REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, nullptr, &hk, nullptr);
    if (!hk) return;
    DWORD dval = 0xDEADBEEF;
    RegSetValueExW(hk, L"DwordW",  0, REG_DWORD,
                   (const BYTE*)&dval, sizeof(dval));
    const char sval[] = "hello_from_A";
    RegSetValueExA(hk, "StringA", 0, REG_SZ,
                   (const BYTE*)sval, (DWORD)sizeof(sval));
    RegCloseKey(hk);
}

static std::wstring T_RegQueryA()
{
    EnsureRegKey();
    HKEY hk = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\HookTestBench",
                      0, KEY_READ, &hk) != ERROR_SUCCESS)
        return L"RegQueryValueExA -- key open failed";
    char buf[64] = {};
    DWORD sz = sizeof(buf), type = 0;
    LONG r = RegQueryValueExA(hk, "StringA", nullptr, &type, (LPBYTE)buf, &sz);
    RegCloseKey(hk);
    if (r == ERROR_SUCCESS) {
        std::wstring ws(buf, buf + strnlen(buf, sizeof(buf)));
        return L"RegQueryValueExA OK  ->  \"" + ws + L"\"";
    }
    return L"RegQueryValueExA failed, code=" + std::to_wstring(r);
}

static std::wstring T_RegQueryW()
{
    EnsureRegKey();
    HKEY hk = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\HookTestBench",
                      0, KEY_READ, &hk) != ERROR_SUCCESS)
        return L"RegQueryValueExW -- key open failed";
    DWORD val = 0, sz = sizeof(val), type = 0;
    LONG r = RegQueryValueExW(hk, L"DwordW", nullptr, &type, (LPBYTE)&val, &sz);
    RegCloseKey(hk);
    if (r == ERROR_SUCCESS) {
        wchar_t res[64];
        swprintf(res, 64, L"RegQueryValueExW OK  ->  0x%08X", val);
        return res;
    }
    return L"RegQueryValueExW failed, code=" + std::to_wstring(r);
}

// ── Registry::Write ───────────────────────────────────────────────────────────
static std::wstring T_RegSetA()
{
    HKEY hk = nullptr; DWORD disp = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\HookTestBench",
                        0, nullptr, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, nullptr, &hk, &disp) != ERROR_SUCCESS)
        return L"RegSetValueExA -- create key failed";
    const char data[] = "written_by_SetValueExA";
    LONG r = RegSetValueExA(hk, "HookTestA", 0, REG_SZ,
                            (const BYTE*)data, (DWORD)sizeof(data));
    RegCloseKey(hk);
    return r == ERROR_SUCCESS
        ? L"RegSetValueExA OK  ->  HKCU\\Software\\HookTestBench\\HookTestA"
        : L"RegSetValueExA failed, code=" + std::to_wstring(r);
}

static std::wstring T_RegSetW()
{
    HKEY hk = nullptr; DWORD disp = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\HookTestBench",
                        0, nullptr, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, nullptr, &hk, &disp) != ERROR_SUCCESS)
        return L"RegSetValueExW -- create key failed";
    DWORD val = 0xCAFEBABE;
    LONG r = RegSetValueExW(hk, L"HookTestW", 0, REG_DWORD,
                            (const BYTE*)&val, sizeof(val));
    RegCloseKey(hk);
    return r == ERROR_SUCCESS
        ? L"RegSetValueExW OK  ->  0xCAFEBABE"
        : L"RegSetValueExW failed, code=" + std::to_wstring(r);
}

// ═════════════════════════════════════════════════════════════════════════════
//  BUTTON LAYOUT
// ═════════════════════════════════════════════════════════════════════════════
struct CatEntry {
    const wchar_t*  name;
    D2D1_COLOR_F    color;
    std::vector<std::pair<const wchar_t*, std::function<std::wstring()>>> items;
};

static void BuildButtons(float winW)
{
    g_btns.clear();

    std::vector<CatEntry> cats = {
        { L"FILE  CREATION",
          Pal::CatFile,
          { {L"CreateFileA",        T_CreateFileA},
            {L"CreateFileW",        T_CreateFileW} } },
        { L"FILE  READ",
          Pal::CatFile,
          { {L"ReadFile",           T_ReadFile},
            {L"ReadFileEx",         T_ReadFileEx} } },
        { L"FILE  WRITE",
          Pal::CatFile,
          { {L"WriteFile",          T_WriteFile},
            {L"WriteFileEx",        T_WriteFileEx} } },
        { L"MESSAGE  BOX",
          Pal::CatMsg,
          { {L"MessageBoxA",        T_MsgBoxA},
            {L"MessageBoxW",        T_MsgBoxW} } },
        { L"DIALOG  BOX",
          Pal::CatDlg,
          { {L"DialogBoxParamA",    T_DialogBoxParamA},
            {L"DialogBoxParamW",    T_DialogBoxParamW},
            {L"CreateDialogParamA", T_CreateDialogParamA},
            {L"CreateDialogParamW", T_CreateDialogParamW} } },
        { L"TASK  DIALOG",
          Pal::CatDlg,
          { {L"TaskDialog",         T_TaskDialog},
            {L"TaskDialogIndirect", T_TaskDialogIndirect} } },
        { L"MEMORY  ALLOC",
          Pal::CatMem,
          { {L"VirtualAlloc",       T_VirtualAlloc},
            {L"VirtualAllocEx",     T_VirtualAllocEx},
            {L"HeapAlloc",          T_HeapAlloc},
            {L"HeapReAlloc",        T_HeapReAlloc} } },
        { L"MEMORY  FREE",
          Pal::CatMem,
          { {L"VirtualFree",        T_VirtualFree},
            {L"VirtualFreeEx",      T_VirtualFreeEx},
            {L"HeapFree",           T_HeapFree} } },
        { L"REGISTRY  READ",
          Pal::CatReg,
          { {L"RegQueryValueExA",   T_RegQueryA},
            {L"RegQueryValueExW",   T_RegQueryW} } },
        { L"REGISTRY  WRITE",
          Pal::CatReg,
          { {L"RegSetValueExA",     T_RegSetA},
            {L"RegSetValueExW",     T_RegSetW} } },
    };

    // Button area: x from (MARGIN + LBL_W + BTN_GX) to (winW - MARGIN)
    float bAreaX = MARGIN + LBL_W + BTN_GX;
    float bAreaW = winW - bAreaX - MARGIN;

    float y = HDR_H + MARGIN;

    for (auto& cat : cats) {
        int n = (int)cat.items.size();
        int perRow = std::min(n, 5);
        float btnW = (bAreaW - (perRow-1)*BTN_GX) / perRow;
        btnW = std::max(btnW, 108.f);

        for (int i = 0; i < n; i++) {
            int col = i % perRow;
            int row = i / perRow;
            float bx = bAreaX + col * (btnW + BTN_GX);
            float by = y + row * (BTN_H + BTN_GY);
            g_btns.push_back({
                cat.items[i].first,
                cat.name,
                {bx, by, bx + btnW, by + BTN_H},
                cat.color,
                cat.items[i].second
            });
        }

        int rows = (n + perRow - 1) / perRow;
        y += rows * (BTN_H + BTN_GY) - BTN_GY + ROW_SEP;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hit-test
// ─────────────────────────────────────────────────────────────────────────────
static int HitTest(float mx, float my)
{
    for (int i = 0; i < (int)g_btns.size(); i++) {
        const auto& r = g_btns[i].rect;
        if (mx >= r.left && mx <= r.right &&
            my >= r.top  && my <= r.bottom)
            return i;
    }
    return -1;
}

// ═════════════════════════════════════════════════════════════════════════════
//  DIRECT2D RESOURCES
// ═════════════════════════════════════════════════════════════════════════════
static void DiscardDeviceRes()
{
    if (g_pBr) { g_pBr->Release(); g_pBr = nullptr; }
    if (g_pRT) { g_pRT->Release(); g_pRT = nullptr; }
}

static HRESULT CreateDeviceRes(HWND hwnd)
{
    if (g_pRT) return S_OK;
    RECT rc; GetClientRect(hwnd, &rc);
    D2D1_SIZE_U sz = {(UINT32)(rc.right-rc.left),(UINT32)(rc.bottom-rc.top)};

    HRESULT hr = g_pD2D->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, sz),
        &g_pRT);
    if (FAILED(hr)) return hr;
    return g_pRT->CreateSolidColorBrush(D2D1::ColorF(0), &g_pBr);
}

// ─── Rendering shortcuts ─────────────────────────────────────────────────────
static void FR(D2D1_RECT_F r, D2D1_COLOR_F c)
{
    g_pBr->SetColor(c);
    g_pRT->FillRectangle(r, g_pBr);
}

static void DT(const std::wstring& s, IDWriteTextFormat* fmt,
               D2D1_RECT_F r, D2D1_COLOR_F c,
               DWRITE_TEXT_ALIGNMENT ha  = DWRITE_TEXT_ALIGNMENT_LEADING,
               DWRITE_PARAGRAPH_ALIGNMENT va = DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
{
    fmt->SetTextAlignment(ha);
    fmt->SetParagraphAlignment(va);
    g_pBr->SetColor(c);
    g_pRT->DrawText(s.c_str(), (UINT32)s.size(), fmt, r, g_pBr);
}

// ═════════════════════════════════════════════════════════════════════════════
//  RENDER
// ═════════════════════════════════════════════════════════════════════════════
static void Render(HWND hwnd)
{
    if (FAILED(CreateDeviceRes(hwnd))) return;

    RECT rc; GetClientRect(hwnd, &rc);
    float W = (float)(rc.right  - rc.left);
    float H = (float)(rc.bottom - rc.top);

    g_pRT->BeginDraw();
    g_pRT->Clear(Pal::BG);

    // ── header ───────────────────────────────────────────────────────────────
    FR({0,0,W,HDR_H}, Pal::Header);
    FR({0,0,4,HDR_H}, Pal::Accent);                     // emerald left stripe
    DT(L"Hook Test Bench",
       g_fTitle, {14,6,W-14,40}, Pal::Text);
    DT(L"Win32 API Hook Trigger  |  Inject your DLL, then click each button",
       g_fSub,   {14,38,W-14,HDR_H-4}, Pal::TextDim);
    FR({0,HDR_H,W,HDR_H+1}, Pal::Divider);

    // ── category labels + left accent bar ────────────────────────────────────
    int i = 0;
    while (i < (int)g_btns.size()) {
        const std::wstring& cn = g_btns[i].catName;
        D2D1_COLOR_F col = g_btns[i].accent;
        float topY = g_btns[i].rect.top;
        float botY = g_btns[i].rect.bottom;
        int j = i;
        while (j+1 < (int)g_btns.size() && g_btns[j+1].catName == cn) {
            botY = std::max(botY, g_btns[j+1].rect.bottom);
            j++;
        }
        FR({MARGIN, topY-2, MARGIN+3, botY+2}, col);
        DT(cn, g_fCat, {MARGIN+7, topY, MARGIN+LBL_W-4, botY},
           {col.r, col.g, col.b, 0.85f});
        i = j+1;
    }

    // ── buttons ──────────────────────────────────────────────────────────────
    for (int k = 0; k < (int)g_btns.size(); k++) {
        const auto& b = g_btns[k];
        bool hov = (k == g_hov);
        FR(b.rect, hov ? Pal::PanelHov : Pal::Panel);
        FR({b.rect.left, b.rect.top, b.rect.left+3, b.rect.bottom}, b.accent);
        if (hov) {
            g_pBr->SetColor({b.accent.r, b.accent.g, b.accent.b, 0.4f});
            g_pRT->DrawRectangle(b.rect, g_pBr, 1.f);
        }
        DT(b.label, g_fBtn,
           {b.rect.left+8, b.rect.top, b.rect.right-4, b.rect.bottom},
           hov ? b.accent : Pal::Text);
    }

    // ── log panel ────────────────────────────────────────────────────────────
    float logY = H - LOG_H;
    FR({0, logY,   W, logY+1}, Pal::Divider);
    FR({0, logY+1, W, H},      Pal::LogBg);
    DT(L"OUTPUT  LOG", g_fCat,
       {MARGIN, logY+4, 160, logY+20}, Pal::TextDim);
    FR({0, logY+21, W, logY+22}, {0.07f,0.09f,0.13f,1.f});

    float ly = logY + 26;
    const float lineH = 17.f;
    for (int li = 0; li < (int)g_log.size() && ly + lineH <= H - 4; li++) {
        DT(g_log[li].text, g_fLog,
           {MARGIN, ly, W-MARGIN, ly+lineH},
           g_log[li].color,
           DWRITE_TEXT_ALIGNMENT_LEADING,
           DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        ly += lineH;
    }

    HRESULT hr = g_pRT->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) DiscardDeviceRes();
}
// static std::wstring SafeRunTest(const Button& b)
// {
//     std::wstring result;
//     __try { result = b.test(); }
//     __except(EXCEPTION_EXECUTE_HANDLER) { result = L"SEH exception!"; }
//     return result;
// }
// ═════════════════════════════════════════════════════════════════════════════
//  WndProc
// ═════════════════════════════════════════════════════════════════════════════
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        AddLog(L"Window created -- ready for injection.", Pal::Accent);
        AddLog(L"Temp dir : " + TempFile(L""),            Pal::TextDim);
        AddLog(L"Reg key  : HKCU\\Software\\HookTestBench", Pal::TextDim);
        return 0;

    case WM_SIZE:
    {
        if (g_pRT) {
            UINT w = LOWORD(lp), h = HIWORD(lp);
            g_pRT->Resize(D2D1::SizeU(w, h));
        }
        BuildButtons((float)LOWORD(lp));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        Render(hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        float mx = (float)(short)LOWORD(lp);
        float my = (float)(short)HIWORD(lp);
        int hov = HitTest(mx, my);
        if (hov != g_hov) {
            g_hov = hov;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        float mx = (float)(short)LOWORD(lp);
        float my = (float)(short)HIWORD(lp);
        int idx = HitTest(mx, my);
        if (idx >= 0 && idx < (int)g_btns.size()) {
            const auto& b = g_btns[idx];
            std::wstring result;
            try {
                result = b.test();
            } catch (...) {
                result = L"Exception caught!";
            }
            AddLog(L"[" + b.catName + L"]  " + b.label + L"  ->  " + result,
                   b.accent);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_hov = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_ERASEBKGND:
        return 1;   // suppress flicker

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ═════════════════════════════════════════════════════════════════════════════
//  WinMain
// ═════════════════════════════════════════════════════════════════════════════
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    g_hInst = hInst;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // D2D factory
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2D))) {
        MessageBoxW(nullptr, L"D2D1CreateFactory failed", L"Error", MB_ICONERROR);
        return 1;
    }

    // DWrite factory
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                   __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(&g_pDW)))) {
        MessageBoxW(nullptr, L"DWriteCreateFactory failed", L"Error", MB_ICONERROR);
        return 1;
    }

    // Text formats
    g_pDW->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 18.f, L"", &g_fTitle);
    g_pDW->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 11.f, L"", &g_fSub);
    g_pDW->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 10.f, L"", &g_fCat);
    g_pDW->CreateTextFormat(L"Segoe UI", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 12.f, L"", &g_fBtn);
    g_pDW->CreateTextFormat(L"Consolas", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 11.f, L"", &g_fLog);

    // Window class
    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"HookTestBenchWnd";
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    RegisterClassExW(&wc);

    // Create window (add WS_EX_COMPOSITED for smoother rendering)
    HWND hwnd = CreateWindowExW(
        0,
        L"HookTestBenchWnd",
        L"Hook Test Bench  --  Win32 API Hook Trigger",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        (int)WIN_W, (int)WIN_H,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) {
        MessageBoxW(nullptr, L"CreateWindowEx failed", L"Error", MB_ICONERROR);
        return 1;
    }

    BuildButtons(WIN_W);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG m = {};
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    // Cleanup
    DiscardDeviceRes();
    if (g_fLog)   g_fLog->Release();
    if (g_fBtn)   g_fBtn->Release();
    if (g_fCat)   g_fCat->Release();
    if (g_fSub)   g_fSub->Release();
    if (g_fTitle) g_fTitle->Release();
    if (g_pDW)    g_pDW->Release();
    if (g_pD2D)   g_pD2D->Release();
    CoUninitialize();
    return (int)m.wParam;
}