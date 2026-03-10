#include "pch.h"
#include "messengerThread.h"
#include "messageQueue.h"
#include <iostream>

std::string ExtractName(const std::string& path)
{
    size_t pos = path.find_last_of("\\/");
    return path.substr(pos + 1);
}

std::string GetProcessName()
{
    char buffer[MAX_PATH];
    DWORD size = MAX_PATH;

    QueryFullProcessImageNameA(
        GetCurrentProcess(),
        0,
        buffer,
        &size
    );

    return ExtractName(std::string(buffer));
}

std::string GetSysTime() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    return std::to_string(st.wHour) + ":" + std::to_string(st.wMinute) + ":" + std::to_string(st.wSecond);
}

DWORD WINAPI MainThread(LPVOID) {
    CreateThread(nullptr, 0, MessageThread, nullptr, 0, nullptr);
    std::string msg = "Dll injected in " + GetProcessName() + " at " + GetSysTime();
    messenger::PutMessage(msg);
    AllocConsole();

    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);

    std::cout << "SpyDll injected successfully!\n";
    std::cout << "Process: " << GetProcessName() << "\n";
    std::cout << "Time: " << GetSysTime() << "\n";
    while (true) {
        std::cout << "Still In\n";
        Sleep(100);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID){
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    if (reason == DLL_THREAD_ATTACH) {}
    if (reason == DLL_THREAD_DETACH) {}
    if (reason == DLL_PROCESS_DETACH) {}
    return TRUE;
}

