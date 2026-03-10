#include "pch.h"
#include "Injector.h"
#include <filesystem>

std::vector<InjectedProc> Injector::InjectedProcesses;

bool Injector::Inject(DWORD pid, const std::string& name)
{
	char fullPath[MAX_PATH];
	GetFullPathNameA("SpyDll.dll", MAX_PATH, fullPath, nullptr);
	std::string dllPath = fullPath;
	static bool checked = false;
	static bool dllExists = false;

	if (!checked){
		dllExists = std::filesystem::exists("SpyDll.dll");
		checked = true;
	}

	if (!dllExists) return false;

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, NULL, pid);
	if (hProc == INVALID_HANDLE_VALUE || hProc == nullptr) return false;
	void* mem = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!mem)
	{
		CloseHandle(hProc);
		return false;
	}
	BOOL b = WriteProcessMemory(hProc, mem, dllPath.c_str(), dllPath.size() + 1, 0);
	if (!b)
	{
		VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return false;
	}
	HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, mem, 0, 0);
	if (!hThread)
	{
		VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return false;
	}
	WaitForSingleObject(hThread, INFINITE);
	HMODULE module = nullptr;
	GetExitCodeThread(hThread, (LPDWORD) &module);
	if (module == NULL) 
	{
		VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return false;
	}
	CloseHandle(hProc);
	CloseHandle(hThread);

	InjectedProc proc;
	proc.mem = mem;
	proc.name = name;
	proc.pid = pid;
	proc.module = module;

	InjectedProcesses.push_back(proc);
	return true;
}

bool Injector::Eject(DWORD pid, const std::string& name)
{
	for (auto it = InjectedProcesses.begin(); it != InjectedProcesses.end(); ++it)
	{
		if (it->pid == pid && it->name == name)
		{
			HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
			if (hProc == INVALID_HANDLE_VALUE || hProc == nullptr) return false;

			HANDLE hThread = CreateRemoteThread(
				hProc,
				nullptr,
				0,
				(LPTHREAD_START_ROUTINE)FreeLibrary,
				it->module,
				0,
				nullptr
			);

			if (!hThread)
			{
				CloseHandle(hProc);
				return false;
			}

			WaitForSingleObject(hThread, INFINITE);
			VirtualFreeEx(hProc, it->mem, 0, MEM_RELEASE);

			CloseHandle(hThread);
			CloseHandle(hProc);

			InjectedProcesses.erase(it);

			return true;
		}
	}

	return false;
}

