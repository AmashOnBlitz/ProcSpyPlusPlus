#include "pch.h"
#include "Injector.h"
#include <filesystem>
#include <Psapi.h>
#include <PipeServer.h>

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

bool Injector::Inject(DWORD pid, const std::string& name)
{
    char fullPath[MAX_PATH];
    GetFullPathNameA("SpyDll.dll", MAX_PATH, fullPath, nullptr);
    std::string sourcePath = fullPath;

    static bool checked = false;
    static bool dllExists = false;
    if (!checked) {
        dllExists = std::filesystem::exists(sourcePath);
        checked = true;
    }
    if (!dllExists) return false;

    std::string dllPath = CreateTempDllCopy(sourcePath);
    if (dllPath.empty()) return false;

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, NULL, pid);
    if (hProc == INVALID_HANDLE_VALUE || hProc == nullptr) return false;

    void* mem = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem)
    {
        CloseHandle(hProc);
        return false;
    }

    BOOL b = WriteProcessMemory(hProc, mem, dllPath.c_str(), dllPath.size() + 1, 0);
    if (!b)
    {
        VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProc, 0, 0,
                                        (LPTHREAD_START_ROUTINE)LoadLibraryA, mem, 0, 0);
    if (!hThread)
    {
        VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    HMODULE modules[1024];
    DWORD needed = 0;
    HMODULE foundModule = nullptr;

    if (EnumProcessModulesEx(hProc, modules, sizeof(modules), &needed, LIST_MODULES_ALL))
    {
        DWORD count = needed / sizeof(HMODULE);
        for (DWORD i = 0; i < count; i++)
        {
            char modName[MAX_PATH];
            if (GetModuleFileNameExA(hProc, modules[i], modName, MAX_PATH))
            {
                if (_stricmp(modName, dllPath.c_str()) == 0)
                {
                    foundModule = modules[i];
                    break;
                }
            }
        }
    }

    if (foundModule == nullptr)
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
        {
            if (mods[i] == hModule) { found = true; break; }
        }
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