#pragma once
#include <Windows.h>
#include <atomic>

inline std::atomic<bool> refreshHooks = true;

DWORD WINAPI HookThread(LPVOID);
void fnRefreshHooks();


void HookFileWrite(bool funcEnabled, bool debEnabled);
void HookMsgBoxCreate(bool funcEnabled, bool debEnabled);