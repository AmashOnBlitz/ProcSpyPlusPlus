#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

struct PipeEntry {
    HANDLE              hPipe = INVALID_HANDLE_VALUE;
    HANDLE              hThread = nullptr;
    HANDLE              hReadEvent = nullptr;
    std::atomic<bool>   active{ false };
    std::atomic<bool>   disconnected{ false }; 
    std::mutex          msgMutex;
    std::vector<std::string> messages;
    std::mutex          writeMutex;
};

struct ThreadArgs {
    DWORD pid;
};

class PipeServer {
public:
    static bool StartListening(DWORD pid);
    static bool SendCommand(DWORD pid, const std::string& cmd);
    static std::vector<std::string> DrainMessages(DWORD pid);
    static bool IsDisconnected(DWORD pid);
    static void Cleanup(DWORD pid);

private:
    static DWORD WINAPI ReaderThread(LPVOID param);

    static std::unordered_map<DWORD, PipeEntry*> s_pipes;
    static std::mutex                             s_mapMutex;
};