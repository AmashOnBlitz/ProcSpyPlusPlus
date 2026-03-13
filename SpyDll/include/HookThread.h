#pragma once
#include <Windows.h>
#include <atomic>

inline std::atomic<bool> refreshHooks = true;

DWORD WINAPI HookThread(LPVOID);
void fnRefreshHooks();

void HookFileCreate(bool funcEnabled, bool debEnabled);
void HookFileRead(bool funcEnabled, bool debEnabled);
void HookFileWrite(bool funcEnabled, bool debEnabled);
void HookMsgBoxCreate(bool funcEnabled, bool debEnabled);
void HookGenericDialogCreate(bool funcEnabled, bool debEnabled);
void HookRegistryRead(bool funcEnabled, bool debEnabled);
void HookRegistryWrite(bool funcEnabled, bool debEnabled);
void HookNetworkSend(bool funcEnabled, bool debEnabled);
void HookNetworkReceive(bool funcEnabled, bool debEnabled);
void HookThreadCreate(bool funcEnabled, bool debEnabled);
void HookDLLLoad(bool funcEnabled, bool debEnabled);
void HookClipboard(bool funcEnabled, bool debEnabled);
void HookScreenshot(bool funcEnabled, bool debEnabled);
void HookWindowCreate(bool funcEnabled, bool debEnabled);
// DEPRECATED: not used - causes deadlock due to re-entrant heap allocations during hook logging
void HookMemAlloc(bool funcEnabled, bool debEnabled);
// DEPRECATED: not used - causes deadlock due to re-entrant heap allocations during hook logging
void HookMemFree(bool funcEnabled, bool debEnabled);