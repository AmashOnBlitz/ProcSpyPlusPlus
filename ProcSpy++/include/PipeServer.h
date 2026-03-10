#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

struct PipeEntry {
    HANDLE      hPipe = INVALID_HANDLE_VALUE;
    HANDLE      hThread = INVALID_HANDLE_VALUE;
    bool        active = true;
    std::vector<std::string> messages;
    std::mutex  msgMutex;
};

class PipeServer {
public:
    static bool  StartListening(DWORD pid);
    static bool  SendCommand(DWORD pid, const std::string& cmd);
    static std::vector<std::string> DrainMessages(DWORD pid);
    static void  Cleanup(DWORD pid);

private:
    static std::unordered_map<DWORD, PipeEntry*> s_pipes;
    static std::mutex                             s_mapMutex;

    struct ThreadArgs { DWORD pid; };
    static DWORD WINAPI ReaderThread(LPVOID param);
};