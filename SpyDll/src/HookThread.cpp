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
			HookFileCreate(APIHookEntry.funcEnabled, APIHookEntry.debug);
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
		case APIHook::HookID::GenericDialogCreate:
		{
			HookGenericDialogCreate(APIHookEntry.funcEnabled, APIHookEntry.debug);
			break;
		}
		case APIHook::HookID::RegistryRead:
		{
			HookRegistryRead(APIHookEntry.funcEnabled, APIHookEntry.debug);
			break;
		}
		case APIHook::HookID::RegistryWrite:
		{
			HookRegistryWrite(APIHookEntry.funcEnabled, APIHookEntry.debug);
			break;
		}
		case APIHook::HookID::MemoryAlloc:
		{
			HookMemAlloc(APIHookEntry.funcEnabled, APIHookEntry.debug);
			break;
		}
		case APIHook::HookID::MemoryFree:
		{
			HookMemFree(APIHookEntry.funcEnabled, APIHookEntry.debug);
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
	static bool fileReadExHooked = false;
	if (!fileReadHooked) {
		MH_CreateHook(
			&ReadFile,
			&HookMethods::File::Read::ReadFileHook,
			reinterpret_cast<void**>(&HookMethods::File::Read::OriginalReadFile)
		);
		fileReadHooked = true;
	}
	if (!fileReadExHooked) {
		MH_CreateHook(
			&ReadFileEx,
			&HookMethods::File::Read::ReadFileExHook,
			reinterpret_cast<void**>(&HookMethods::File::Read::OriginalReadFileEx)
		);
		fileReadExHooked = true;
	}
}

void HookFileWrite(bool funcEnabled, bool debEnabled)
{
	HookMethods::File::Write::DebugEnabled = debEnabled;
	HookMethods::File::Write::WriteEnabled = funcEnabled;
	static bool fileWriteHooked = false;
	static bool fileWriteExHooked = false;
	if (!fileWriteHooked) {
		MH_CreateHook(
			&WriteFile,
			&HookMethods::File::Write::WriteFileHook,
			reinterpret_cast<void**>(&HookMethods::File::Write::OriginalWriteFile)
		);
		fileWriteHooked = true;
	}
	if (!fileWriteExHooked) {
		MH_CreateHook(
			&WriteFileEx,
			&HookMethods::File::Write::WriteFileExHook,
			reinterpret_cast<void**>(&HookMethods::File::Write::OriginalWriteFileEx)
		);
		fileWriteExHooked = true;
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

void HookGenericDialogCreate(bool funcEnabled, bool debEnabled)
{
	HookMethods::Dialog::DebugEnabled = debEnabled;
	HookMethods::Dialog::DialogEnabled = funcEnabled;
	static bool dialogBoxParamHooked = false;
	static bool createDialogParamHooked = false;
	static bool taskDialogHooked = false;
	static bool taskDialogIndirectHooked = false;
	if (!dialogBoxParamHooked) {
		MH_CreateHook(
			&DialogBoxParamW,
			&HookMethods::Dialog::DialogBoxParamWHook,
			reinterpret_cast<void**>(&HookMethods::Dialog::OriginalDialogBoxParamW)
		);
		MH_CreateHook(
			&DialogBoxParamA,
			&HookMethods::Dialog::DialogBoxParamAHook,
			reinterpret_cast<void**>(&HookMethods::Dialog::OriginalDialogBoxParamA)
		);
		dialogBoxParamHooked = true;
	}
	if (!createDialogParamHooked) {
		MH_CreateHook(
			&CreateDialogParamW,
			&HookMethods::Dialog::CreateDialogParamWHook,
			reinterpret_cast<void**>(&HookMethods::Dialog::OriginalCreateDialogParamW)
		);
		MH_CreateHook(
			&CreateDialogParamA,
			&HookMethods::Dialog::CreateDialogParamAHook,
			reinterpret_cast<void**>(&HookMethods::Dialog::OriginalCreateDialogParamA)
		);
		createDialogParamHooked = true;
	}
	static HMODULE hComCtl = NULL;
	if (!hComCtl) hComCtl = GetModuleHandleA("comctl32.dll");
	if (!hComCtl) hComCtl = LoadLibraryA("comctl32.dll");

	if (!taskDialogHooked && hComCtl) {
		void* pTaskDialog = GetProcAddress(hComCtl, "TaskDialog");
		if (pTaskDialog) {
			MH_CreateHook(
				pTaskDialog,
				&HookMethods::Dialog::TaskDialogHook,
				reinterpret_cast<void**>(&HookMethods::Dialog::OriginalTaskDialog)
			);
			taskDialogHooked = true;
		}
	}
	if (!taskDialogIndirectHooked && hComCtl) {
		void* pTaskDialogIndirect = GetProcAddress(hComCtl, "TaskDialogIndirect");
		if (pTaskDialogIndirect) {
			MH_CreateHook(
				pTaskDialogIndirect,
				&HookMethods::Dialog::TaskDialogIndirectHook,
				reinterpret_cast<void**>(&HookMethods::Dialog::OriginalTaskDialogIndirect)
			);
			taskDialogIndirectHooked = true;
		}
	}
}

void HookRegistryRead(bool funcEnabled, bool debEnabled)
{
	HookMethods::Registry::Read::DebugEnabled = debEnabled;
	HookMethods::Registry::Read::ReadEnabled = funcEnabled;
	static bool regReadHooked = false;
	if (!regReadHooked) {
		MH_CreateHook(
			&RegQueryValueExW,
			&HookMethods::Registry::Read::RegQueryValueExWHook,
			reinterpret_cast<void**>(&HookMethods::Registry::Read::OriginalRegQueryValueExW)
		);
		MH_CreateHook(
			&RegQueryValueExA,
			&HookMethods::Registry::Read::RegQueryValueExAHook,
			reinterpret_cast<void**>(&HookMethods::Registry::Read::OriginalRegQueryValueExA)
		);
		regReadHooked = true;
	}
}

void HookRegistryWrite(bool funcEnabled, bool debEnabled)
{
	HookMethods::Registry::Write::DebugEnabled = debEnabled;
	HookMethods::Registry::Write::WriteEnabled = funcEnabled;
	static bool regWriteHooked = false;
	if (!regWriteHooked) {
		MH_CreateHook(
			&RegSetValueExW,
			&HookMethods::Registry::Write::RegSetValueExWHook,
			reinterpret_cast<void**>(&HookMethods::Registry::Write::OriginalRegSetValueExW)
		);
		MH_CreateHook(
			&RegSetValueExA,
			&HookMethods::Registry::Write::RegSetValueExAHook,
			reinterpret_cast<void**>(&HookMethods::Registry::Write::OriginalRegSetValueExA)
		);
		regWriteHooked = true;
	}
}

void HookMemAlloc(bool funcEnabled, bool debEnabled)
{
	HookMethods::Memory::Alloc::AllocEnabled = funcEnabled;
	HookMethods::Memory::Alloc::DebugEnabled = debEnabled;
	static bool memAllocHooked = false;
	if (!memAllocHooked) {
		MH_CreateHook(
			&VirtualAlloc,
			&HookMethods::Memory::Alloc::VirtualAllocHook,
			reinterpret_cast<void**>(&HookMethods::Memory::Alloc::originalVirtualAlloc)
		);
		MH_CreateHook(
			&VirtualAllocEx,
			&HookMethods::Memory::Alloc::VirtualAllocExHook,
			reinterpret_cast<void**>(&HookMethods::Memory::Alloc::originalVirtualAllocEx)
		);
		MH_CreateHook(
			&HeapAlloc,
			&HookMethods::Memory::Alloc::HeapAllocHook,
			reinterpret_cast<void**>(&HookMethods::Memory::Alloc::originalHeapAlloc)
		);
		MH_CreateHook(
			&HeapReAlloc,
			&HookMethods::Memory::Alloc::HeapReAllocHook,
			reinterpret_cast<void**>(&HookMethods::Memory::Alloc::originalHeapReAlloc)
		);
		memAllocHooked = true;
	}
}

void HookMemFree(bool funcEnabled, bool debEnabled)
{
	HookMethods::Memory::Free::FreeEnabled = funcEnabled;
	HookMethods::Memory::Free::DebugEnabled = debEnabled;
	static bool memFreeHooked = false;
	if (!memFreeHooked) {
		MH_CreateHook(
			&VirtualFree,
			&HookMethods::Memory::Free::VirtualFreeHook,
			reinterpret_cast<void**>(&HookMethods::Memory::Free::originalVirtualFree)
		);
		MH_CreateHook(
			&VirtualFreeEx,
			&HookMethods::Memory::Free::VirtualFreeExHook,
			reinterpret_cast<void**>(&HookMethods::Memory::Free::originalVirtualFreeEx)
		);
		MH_CreateHook(
			&HeapFree,
			&HookMethods::Memory::Free::HeapFreeHook,
			reinterpret_cast<void**>(&HookMethods::Memory::Free::originalHeapFree)
		);
		memFreeHooked = true;
	}
}
