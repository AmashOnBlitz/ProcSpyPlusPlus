#include "pch.h"
#include "HookThread.h"
#include "HookMethods.h"
#include "Flags.h"
#include "APIHook.h"
#include <MinHook.h>

DWORD WINAPI HookThread(LPVOID) {
	while (!ThreadExpectedToStop) {
		if (refreshHooks) {
			OutputDebugStringA("Got Refresh Hooks");
			fnRefreshHooks();
			refreshHooks = false;
		}
		Sleep(50);
	}
	return 0;
}

void fnRefreshHooks()
{
	static bool isMHInit = false;
	if (!isMHInit) {
		if (MH_Initialize() == MH_OK) {
			isMHInit = true;
		}
	}
	std::lock_guard<std::mutex> lock(APIHook::APITableMutex);
	for (APIHook::APIFunctionsStruct& APIHookEntry : APIHook::APIFunctions) {
		switch (APIHookEntry.id)
		{
		case APIHook::HookID::FileCreate:
		{
			HookFileCreate(APIHookEntry.funcEnabled,APIHookEntry.debug);
			break;
		}
		case APIHook::HookID::FileRead:
		{
			HookFileRead(APIHookEntry.funcEnabled, APIHookEntry.debug);
			break;
		}
		case APIHook::HookID::FileWrite:
		{
			HookFileWrite(APIHookEntry.funcEnabled, APIHookEntry.debug);
			break;
		}
		case APIHook::HookID::MessageBoxCreate:
		{
			HookMsgBoxCreate(APIHookEntry.funcEnabled, APIHookEntry.debug);
			break;
		}
		default:
			break;
		}
	}
	static bool MHHooksEnabled = false;
	if (!MHHooksEnabled) {
		if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK) {
			MHHooksEnabled = true;
		}
		else {
		}
	}
}

void HookFileCreate(bool funcEnabled, bool debEnabled)
{
	HookMethods::File::Creation::DebugEnabled = debEnabled;
	HookMethods::File::Creation::CreateEnabled = funcEnabled;
	static bool fileCreateHooked = false;
	if (!fileCreateHooked) {
		MH_CreateHook(
			&CreateFileW,
			&HookMethods::File::Creation::CreateFileWHook,
			reinterpret_cast<void**>(&HookMethods::File::Creation::OriginalCreateFileW)
		);
		MH_CreateHook(
			&CreateFileA,
			&HookMethods::File::Creation::CreateFileAHook,
			reinterpret_cast<void**>(&HookMethods::File::Creation::OriginalCreateFileA)
		);
		fileCreateHooked = true;
	}
}

void HookFileRead(bool funcEnabled, bool debEnabled)
{
	HookMethods::File::Read::DebugEnabled = debEnabled;
	HookMethods::File::Read::ReadEnabled = funcEnabled;
	static bool fileReadHooked = false;
	if (!fileReadHooked) {
		MH_CreateHook(
			&ReadFile,
			&HookMethods::File::Read::ReadFileHook,
			reinterpret_cast<void**>(&HookMethods::File::Read::OriginalReadFile)
		);
		fileReadHooked = true;
	}
}

void HookFileWrite(bool funcEnabled, bool debEnabled)
{
	HookMethods::File::Write::DebugEnabled = debEnabled;
	HookMethods::File::Write::WriteEnabled = funcEnabled;

	static bool fileWriteHooked = false;
	if (!fileWriteHooked) {
		MH_CreateHook(
			&WriteFile,
			&HookMethods::File::Write::WriteFileHook,
			reinterpret_cast<void**>(&HookMethods::File::Write::OriginalWriteFile)
		);
		fileWriteHooked = true;
	}
}

void HookMsgBoxCreate(bool funcEnabled, bool debEnabled)
{
	HookMethods::MsgBox::DebugEnabled = debEnabled;
	HookMethods::MsgBox::MsgBoxEnabled = funcEnabled;
	static bool msgBoxHooked = false;
	if (!msgBoxHooked) {
		MH_CreateHook(
			&MessageBoxW,
			&HookMethods::MsgBox::MessageBoxWHook,
			reinterpret_cast<void**>(&HookMethods::MsgBox::OriginalMessageBoxW)
		);
		MH_CreateHook(
			&MessageBoxA,
			&HookMethods::MsgBox::MessageBoxAHook,
			reinterpret_cast<void**>(&HookMethods::MsgBox::OriginalMessageBoxA)
		);
		msgBoxHooked = true;
	}
}
