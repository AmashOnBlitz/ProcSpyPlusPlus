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

    static inline std::vector<APIFunctionsStruct> APIFunctions = {
        {HookID::RegistryRead, "Registry Read", false, false},
        {HookID::RegistryWrite, "Registry Write", false, false},
        {HookID::FileRead, "File Read", false, false},
        {HookID::FileWrite, "File Write", false, false},
        {HookID::NetworkSend, "Network Send", false, false},
        {HookID::NetworkReceive, "Network Receive", false, false},
        {HookID::MessageBoxCreate, "Message Box Create", false, false},
        {HookID::ThreadCreate, "Thread Create", false, false},
        {HookID::MemoryAlloc, "Memory Alloc", false, false},
        {HookID::DLLLoad, "DLL Load", false, false},
        {HookID::ClipboardAccess, "Clipboard Access", false, false},
        {HookID::ScreenshotCapture, "Screenshot Capture", false, false},
    };

    bool setHook(const std::string& name, bool state, bool isEnabledFlag, std::string& err);
    bool updateHookTable(const std::string& name, bool state, bool isEnabledFlag, std::string& err);
}
