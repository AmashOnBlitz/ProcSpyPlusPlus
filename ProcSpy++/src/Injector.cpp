#include "pch.h"
#include "Injector.h"
#include <filesystem>
#include <Psapi.h>
#include <PipeServer.h>
#include <tlhelp32.h>

std::vector<InjectedProc> Injector::InjectedProcesses;

static std::string CreateTempDllCopy(const std::string& sourcePath)
{
    char tempDir[MAX_PATH];
    if (!GetTempPathA(MAX_PATH, tempDir))
        return "";

    std::string uniqueName = "SpyPlusPlusDLLAutoDeleteSheduled_"
        + std::to_string(GetCurrentProcessId())
        + "_"
        + std::to_string(GetTickCount64())
        + ".dll";

    std::string tempPath = std::string(tempDir) + uniqueName;

    if (!CopyFileA(sourcePath.c_str(), tempPath.c_str(), FALSE))
        return "";

    MoveFileExA(tempPath.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
    return tempPath;
}

static void* AllocAndWritePath(HANDLE hProc, const std::string& dllPath)
{
    void* mem = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) return nullptr;
    if (!WriteProcessMemory(hProc, mem, dllPath.c_str(), dllPath.size() + 1, 0))
    {
        VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
        return nullptr;
    }
    return mem;
}

static HMODULE FindLoadedModule(HANDLE hProc, const std::string& dllPath)
{
    HMODULE modules[1024];
    DWORD needed = 0;
    if (!EnumProcessModulesEx(hProc, modules, sizeof(modules), &needed, LIST_MODULES_ALL))
        return nullptr;
    for (DWORD i = 0, n = needed / sizeof(HMODULE); i < n; i++)
    {
        char name[MAX_PATH];
        if (GetModuleFileNameExA(hProc, modules[i], name, MAX_PATH))
            if (_stricmp(name, dllPath.c_str()) == 0) return modules[i];
    }
    return nullptr;
}

static bool TryCreateRemoteThread(HANDLE hProc, void* mem)
{
    HANDLE h = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, mem, 0, 0);
    if (!h) return false;
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    return true;
}

static bool TryQueueUserAPC(HANDLE hProc, void* mem, DWORD pid, const std::string& dllPath)
{
    FARPROC loadLib = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!loadLib) return false;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    THREADENTRY32 te{}; te.dwSize = sizeof(te);
    if (Thread32First(snap, &te))
    {
        do
        {
            if (te.th32OwnerProcessID != pid) continue;
            HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, te.th32ThreadID);
            if (!hThread) continue;
            QueueUserAPC((PAPCFUNC)loadLib, hThread, (ULONG_PTR)mem);
            CloseHandle(hThread);
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);

    for (int i = 0; i < 30; i++)
    {
        Sleep(100);
        if (FindLoadedModule(hProc, dllPath)) return true;
    }
    return false;
}

bool Injector::Inject(DWORD pid, const std::string& name)
{
    char fullPath[MAX_PATH];
    GetFullPathNameA("SpyDll.dll", MAX_PATH, fullPath, nullptr);
    std::string sourcePath = fullPath;

    static bool checked = false;
    static bool dllExists = false;
    if (!checked) { dllExists = std::filesystem::exists(sourcePath); checked = true; }
    if (!dllExists) return false;

    std::string dllPath = CreateTempDllCopy(sourcePath);
    if (dllPath.empty()) return false;

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, NULL, pid);
    if (!hProc || hProc == INVALID_HANDLE_VALUE) return false;

    void* mem = AllocAndWritePath(hProc, dllPath);
    if (!mem) { CloseHandle(hProc); return false; }

    HMODULE foundModule = nullptr;

    if (TryCreateRemoteThread(hProc, mem))
        foundModule = FindLoadedModule(hProc, dllPath);

    if (!foundModule && TryQueueUserAPC(hProc, mem, pid, dllPath))
        foundModule = FindLoadedModule(hProc, dllPath);

    if (!foundModule)
    {
        VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    CloseHandle(hProc);

    InjectedProc proc;
    proc.mem = mem;
    proc.name = name;
    proc.pid = pid;
    proc.module = foundModule;
    proc.tempDllPath = dllPath;
    InjectedProcesses.push_back(proc);

    PipeServer::StartListening(pid);
    return true;
}

bool Injector::Eject(DWORD pid, const std::string& name)
{
    return Deactivate(pid, name);
    // CODE BELOW IS DEPRECATED AS IT CAUSED UNEXPECTED ERRORS ---
    //for (auto it = InjectedProcesses.begin(); it != InjectedProcesses.end(); ++it)
    //{
    //    if (it->pid == pid && it->name == name)
    //    {
    //        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    //        if (hProc == INVALID_HANDLE_VALUE || hProc == nullptr) return false;
    //        HANDLE hThread = CreateRemoteThread(
    //            hProc,
    //            nullptr,
    //            0,
    //            (LPTHREAD_START_ROUTINE)FreeLibrary,
    //            it->module,
    //            0,
    //            nullptr
    //        );
    //        if (!hThread)
    //        {
    //            CloseHandle(hProc);
    //            return false;
    //        }
    //        WaitForSingleObject(hThread, INFINITE);
    //        BOOL res = VirtualFreeEx(hProc, it->mem, 0, MEM_RELEASE);
    //        BOOL IsThreadHandleClosed = CloseHandle(hThread);
    //        BOOL IsProcHandleClosed = CloseHandle(hProc);
    //        InjectedProcesses.erase(it);
    //        return true;
    //    }
    //}
    //return false;
}

static bool IsProcessAlive(DWORD pid)
{
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;
    DWORD code = 0;
    bool alive = GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
    CloseHandle(h);
    return alive;
}

static bool IsModuleStillLoaded(DWORD pid, HMODULE hModule)
{
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) return false;

    HMODULE mods[1024];
    DWORD needed = 0;
    bool found = false;
    if (EnumProcessModulesEx(hProc, mods, sizeof(mods), &needed, LIST_MODULES_ALL))
    {
        DWORD count = needed / sizeof(HMODULE);
        for (DWORD i = 0; i < count; i++)
            if (mods[i] == hModule) { found = true; break; }
    }
    CloseHandle(hProc);
    return found;
}

bool Injector::Deactivate(DWORD pid, const std::string& name)
{
    auto it = std::find_if(InjectedProcesses.begin(), InjectedProcesses.end(),
                           [&](const InjectedProc& p) { return p.pid == pid && p.name == name; });
    if (it == InjectedProcesses.end()) return false;

    HMODULE savedModule = it->module;

    if (!IsProcessAlive(pid))
    {
        PipeServer::Cleanup(pid);
        InjectedProcesses.erase(it);
        return true;
    }

    PipeServer::SendCommand(pid, "STOP");
    Sleep(300);
    PipeServer::Cleanup(pid);

    bool moduleGone = !IsModuleStillLoaded(pid, savedModule);
    bool processGone = !IsProcessAlive(pid);

    InjectedProcesses.erase(it);

    return moduleGone || processGone || true;
}

//DWORD getPidFromName(const char* exeName) {
//    DWORD pid = 0;
//    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//    while (hSnapshot != INVALID_HANDLE_VALUE) {
//        WCHAR wideExeName[MAX_PATH];
//        MultiByteToWideChar(CP_ACP, 0, exeName, -1, wideExeName, MAX_PATH);
//        PROCESSENTRY32W proc32;
//        proc32.dwSize = sizeof(PROCESSENTRY32W);
//        if (Process32FirstW(hSnapshot, &proc32)) {
//            do {
//                if (wcscmp(proc32.szExeFile, wideExeName) == 0) {
//                    pid = proc32.th32ParentProcessID;
//                    break;
//                }
//            } while (Process32NextW(hSnapshot, &proc32));
//        }
//    }
//    return pid;
//}

bool Injector::LoadCustomDll(DWORD _pid, const char* exe, const char* dllPath)
{
    DWORD pid = _pid;
    if (pid == 0)  return FALSE;
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, NULL, pid);
    if (!hProc || hProc == INVALID_HANDLE_VALUE) return FALSE;
    void* mem = VirtualAllocEx(hProc, NULL, sizeof(dllPath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (mem == nullptr) {
        CloseHandle(hProc);
        return FALSE;
    }
    BOOL writtenProcMem = WriteProcessMemory(hProc, mem, dllPath, strlen(dllPath) + 1, 0);
    if (writtenProcMem == FALSE) {
        CloseHandle(hProc);
        return FALSE;
    }
    HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)(LoadLibraryA), mem, 0, 0);
    if (!hThread || hThread == INVALID_HANDLE_VALUE) {
        Sleep(100);
        if (hProc) CloseHandle(hProc);
        if (hThread) CloseHandle(hThread);
        return FALSE;
    }
    CloseHandle(hProc);
    CloseHandle(hThread);
    return TRUE;
}
