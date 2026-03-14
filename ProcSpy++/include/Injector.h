#pragma once
#include <Windows.h>
#include <string>
#include <vector>

struct InjectedProc {
	DWORD pid;
	std::string name;
	void* mem = nullptr;
	HMODULE module;
	std::string tempDllPath;
};

class Injector final {

public:

	Injector() = default;
	static bool Inject(DWORD pid, const std::string& name);
	static bool Eject(DWORD pid, const std::string& name);
	static bool Deactivate(DWORD pid, const std::string& name);
	static bool LoadCustomDll(DWORD _pid, const char* exe, const char* dllPath);
	static std::vector<InjectedProc> InjectedProcesses;
};