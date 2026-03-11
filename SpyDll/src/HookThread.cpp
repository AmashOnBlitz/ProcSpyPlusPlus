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
		Sleep(100);
	}
	return 0;
}

inline void fnRefreshHooks()
{
	static bool isMHInit = false;
	if (!isMHInit) {
		if (MH_Initialize() == MH_OK) {
			isMHInit = true;
			std::cout << "MH Init Done\n";
		}
	}
	std::lock_guard<std::mutex> lock(APIHook::APITableMutex);
	for (APIHook::APIFunctionsStruct& APIHookEntry : APIHook::APIFunctions) {
		switch (APIHookEntry.id)
		{
		case APIHook::HookID::FileWrite:
		{
			HookFileWrite(APIHookEntry.funcEnabled,APIHookEntry.debug);
			std::cout << "Hooked File Write\n";
			std::cout << (("Enabled: " + std::string(APIHookEntry.funcEnabled ? "true" : "false") + " Debug: " + std::string(APIHookEntry.debug ? "true" : "false")).c_str()) << "\n";
			break;
		}
		case APIHook::HookID::MessageBoxCreate:
		{
			HookMsgBoxCreate(APIHookEntry.funcEnabled, APIHookEntry.debug);
			std::cout << "Hooked Msg Box";
			std::cout << ("Enabled: " + std::string(APIHookEntry.funcEnabled ? "true" : "false") + " Debug: " + std::string(APIHookEntry.debug ? "true" : "false")) << "\n";
			break;
		}
		default:
			break;
		}
	}
	static bool MHHooksInit = false;
	if (!MHHooksInit) {
		if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK) {
			MHHooksInit = true;
			std::cout << "All Hooks Init\n";
		}
		else {
			std::cout << "All Hooks Init Failed!\n";
		}
	}
}

inline void HookFileWrite(bool funcEnabled, bool debEnabled)
{
	HookMethods::File::Write::DebugEnabled = debEnabled;
	HookMethods::File::Write::WriteEnabled = funcEnabled;
	static bool fileHooked = false;
	if (!fileHooked) {
		MH_CreateHook(
			&CreateFileW,
			&HookMethods::File::Write::CreateFileWHook,
			reinterpret_cast<void**>(&HookMethods::File::Write::OriginalCreateFileW)
		);
		MH_CreateHook(
			&CreateFileA,
			&HookMethods::File::Write::CreateFileAHook,
			reinterpret_cast<void**>(&HookMethods::File::Write::OriginalCreateFileA)
		);
		fileHooked = true;
	}
}

inline void HookMsgBoxCreate(bool funcEnabled, bool debEnabled)
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
