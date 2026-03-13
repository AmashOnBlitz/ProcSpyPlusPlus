#include "pch.h"
#include "messengerThread.h"
#include "Flags.h"
#include "messageQueue.h"
#include <string>
#include <CMDHandler.h>
#include <iostream>
#define CMDSTRING "#cmd|"

static std::string GetPipeName() {
    return "\\\\.\\pipe\\messagePipeline_" + std::to_string(GetCurrentProcessId());
}

DWORD WINAPI MessageThread(LPVOID) {
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    int failureIteration = 1;
    MessagePipelineAllocMem = true;
    do {
        hPipe = CreateFileA(
            GetPipeName().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, NULL,
            OPEN_EXISTING,
            0, NULL
        );
        Sleep(std::clamp(50 * failureIteration, 50, 5000));
        failureIteration++;
    } while (hPipe == INVALID_HANDLE_VALUE && !ThreadExpectedToStop);

    if (hPipe == INVALID_HANDLE_VALUE) return 1;
    DWORD mode = PIPE_READMODE_MESSAGE;
    SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);

    std::string buff;
    DWORD bytesWritten = 0;

    while (!ThreadExpectedToStop) {
        MessagePipelineAllocMem = true;
        while (messenger::readOffSet < messenger::writeOffSet) {
            buff = messenger::ReadMessage();
            if (!buff.empty()) {
                WriteFile(hPipe, buff.c_str(),
                          static_cast<DWORD>(buff.size()),
                          &bytesWritten, NULL);
            }
        }
        DWORD available = 0;
        if (PeekNamedPipe(hPipe, NULL, 0, NULL, &available, NULL) && available > 0) {
            char cmdBuf[2048] = {};
            DWORD bytesRead = 0;
            if (ReadFile(hPipe, cmdBuf, sizeof(cmdBuf) - 1, &bytesRead, NULL)) {
                std::string cmd(cmdBuf, bytesRead);
                if (cmd == "STOP") {
                    ThreadExpectedToStop = true;
                    const char* ack = "ACK:STOPPED";
                    WriteFile(hPipe, ack, (DWORD)strlen(ack), &bytesWritten, NULL);
                }
                else if (cmd.rfind(CMDSTRING, 0) == 0) {
                    extractCMD(cmd);
                }
            }
        }
        MessagePipelineAllocMem = false;
        Sleep(20);
    }
    MessagePipelineAllocMem = false;
    CloseHandle(hPipe);
    return 0;
}