#include "pch.h"
#include "PipeServer.h"
#include <string>

std::unordered_map<DWORD, PipeEntry*> PipeServer::s_pipes;
std::mutex                            PipeServer::s_mapMutex;

bool PipeServer::StartListening(DWORD pid)
{
    std::string pipeName = "\\\\.\\pipe\\messagePipeline_" + std::to_string(pid);

    HANDLE hPipe = CreateNamedPipeA(
        pipeName.c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        4096, 4096,
        0, nullptr
    );
    if (hPipe == INVALID_HANDLE_VALUE) return false;

    HANDLE hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!hEvent) {
        CloseHandle(hPipe);
        return false;
    }

    auto* entry = new PipeEntry();
    entry->hPipe = hPipe;
    entry->hReadEvent = hEvent;
    entry->active = true;

    {
        std::lock_guard<std::mutex> lk(s_mapMutex);
        s_pipes[pid] = entry;
    }

    auto* args = new ThreadArgs{ pid };
    HANDLE hThr = CreateThread(nullptr, 0, ReaderThread, args, 0, nullptr);
    if (!hThr) {
        CloseHandle(hPipe);
        CloseHandle(hEvent);
        delete entry;
        delete args;
        std::lock_guard<std::mutex> lk(s_mapMutex);
        s_pipes.erase(pid);
        return false;
    }

    entry->hThread = hThr;
    return true;
}

DWORD WINAPI PipeServer::ReaderThread(LPVOID param)
{
    auto* args = reinterpret_cast<ThreadArgs*>(param);
    DWORD pid = args->pid;
    delete args;

    PipeEntry* entry = nullptr;
    {
        std::lock_guard<std::mutex> lk(s_mapMutex);
        auto it = s_pipes.find(pid);
        if (it == s_pipes.end()) return 1;
        entry = it->second;
    }

    BOOL connected = ConnectNamedPipe(entry->hPipe, nullptr);
    if (!connected && GetLastError() != ERROR_PIPE_CONNECTED) return 1;

    char buf[4096];

    while (entry->active)
    {
        OVERLAPPED ov = {};
        ov.hEvent = entry->hReadEvent;
        ResetEvent(entry->hReadEvent);

        DWORD bytesRead = 0;
        BOOL  ok = ReadFile(entry->hPipe, buf, sizeof(buf) - 1, &bytesRead, &ov);

        if (!ok && GetLastError() == ERROR_IO_PENDING)
        {
            while (entry->active)
            {
                DWORD wait = WaitForSingleObject(entry->hReadEvent, 100);
                if (wait == WAIT_OBJECT_0) break;
            }

            if (!entry->active)
            {
                CancelIo(entry->hPipe);
                break;
            }

            ok = GetOverlappedResult(entry->hPipe, &ov, &bytesRead, FALSE);
        }

        if (!ok || bytesRead == 0) break;

        buf[bytesRead] = '\0';
        std::string msg(buf, bytesRead);

        if (msg == "ACK:STOPPED") {
            std::lock_guard<std::mutex> lk(entry->msgMutex);
            entry->messages.push_back("[system] DLL acknowledged stop");
            break;
        }

        std::lock_guard<std::mutex> lk(entry->msgMutex);
        entry->messages.push_back(msg);
    }

    DisconnectNamedPipe(entry->hPipe);
    return 0;
}

bool PipeServer::SendCommand(DWORD pid, const std::string& cmd)
{
    std::lock_guard<std::mutex> lk(s_mapMutex);
    auto it = s_pipes.find(pid);
    if (it == s_pipes.end()) return false;

    PipeEntry* entry = it->second;
    std::lock_guard<std::mutex> wlk(entry->writeMutex);

    DWORD written = 0;
    return WriteFile(entry->hPipe,
                     cmd.c_str(), static_cast<DWORD>(cmd.size()),
                     &written, nullptr);
}

std::vector<std::string> PipeServer::DrainMessages(DWORD pid)
{
    std::lock_guard<std::mutex> lk(s_mapMutex);
    auto it = s_pipes.find(pid);
    if (it == s_pipes.end()) return {};

    PipeEntry* entry = it->second;
    std::lock_guard<std::mutex> mlk(entry->msgMutex);
    std::vector<std::string> out = std::move(entry->messages);
    entry->messages.clear();
    return out;
}

void PipeServer::Cleanup(DWORD pid)
{
    std::lock_guard<std::mutex> lk(s_mapMutex);
    auto it = s_pipes.find(pid);
    if (it == s_pipes.end()) return;

    PipeEntry* entry = it->second;
    entry->active = false;

    CloseHandle(entry->hReadEvent);
    CloseHandle(entry->hPipe);
    CloseHandle(entry->hThread);
    delete entry;
    s_pipes.erase(it);
}