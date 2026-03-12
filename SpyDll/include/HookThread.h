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
void HookMemAlloc(bool funcEnabled, bool debEnabled);
void HookMemFree(bool funcEnabled, bool debEnabled);