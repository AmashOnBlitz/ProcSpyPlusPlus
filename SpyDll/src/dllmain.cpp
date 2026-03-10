#include "pch.h"
#include "messengerThread.h"
#include "messageQueue.h"
#include <iostream>
#include <atomic>
#include <Flags.h>
std::atomic<bool> running = true;

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

DWORD WINAPI NetworkMonitorThread(LPVOID) {
    HANDLE hDevice = nullptr;

    while (!ThreadExpectedToStop) {
        messenger::PutMessage("network event");

        Sleep(1000);
    }
    if (hDevice) CloseHandle(hDevice);
    return 0;
}

DWORD WINAPI MainThread(LPVOID) {
    CreateThread(nullptr, 0, MessageThread, nullptr, 0, nullptr);
    std::string msg = "Dll injected in " + GetProcessName() + " at " + GetSysTime();
    messenger::PutMessage(msg);
    CreateThread(nullptr, 0, NetworkMonitorThread, nullptr, 0, nullptr);
    return 0;
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID){
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    if (reason == DLL_THREAD_ATTACH) {}
    if (reason == DLL_THREAD_DETACH) {}
    if (reason == DLL_PROCESS_DETACH)
    {
        running = false;
    }
    return TRUE;
}