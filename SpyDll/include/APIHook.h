#pragma once
#include <vector>
#include <string>
#include <mutex>

namespace APIHook {
    inline std::mutex APITableMutex;

    enum class HookID
    {
        RegistryRead,
        RegistryWrite,
        FileRead,
        FileWrite,
        NetworkSend,
        NetworkReceive,
        MessageBoxCreate,
        ThreadCreate,
        MemoryAlloc,
        DLLLoad,
        ClipboardAccess,
        ScreenshotCapture
    };

    struct APIFunctionsStruct
    {
        HookID id;
        const char* label;
        bool funcEnabled;
        bool debug;
    };

    inline std::vector<APIFunctionsStruct> APIFunctions = {
        {HookID::RegistryRead, "Registry Read", true, false},
        {HookID::RegistryWrite, "Registry Write", true, false},
        {HookID::FileRead, "File Read", true, false},
        {HookID::FileWrite, "File Write", true, false},
        {HookID::NetworkSend, "Network Send", true, false},
        {HookID::NetworkReceive, "Network Receive", true, false},
        {HookID::MessageBoxCreate, "Message Box Create", true, false},
        {HookID::ThreadCreate, "Thread Create", true, false},
        {HookID::MemoryAlloc, "Memory Alloc", true, false},
        {HookID::DLLLoad, "DLL Load", true, false},
        {HookID::ClipboardAccess, "Clipboard Access", true, false},
        {HookID::ScreenshotCapture, "Screenshot Capture", true, false},
    };

    bool setHook(const std::string& name, bool state, bool isEnabledFlag, std::string& err);
    bool updateHookTable(const std::string& name, bool state, bool isEnabledFlag, std::string& err);
}