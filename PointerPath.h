#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <vector>



class PointerPath32
{
private:

	std::vector<int> offsets;
	static DWORD GetStackStart(_In_ HANDLE process, DWORD pId, _In_ HANDLE thread);
	static _Success_(return) bool GetModuleInfo(DWORD pId, _In_ TCHAR * moduleName, _Out_ MODULEENTRY32 &moduleInfo);

public:

	PointerPath32(std::vector<int> offsets);
	~PointerPath32();
	bool _Success_(return) Deref(_In_ HANDLE process, DWORD base, _Out_ long &value);
	std::string Deref(_In_ HANDLE process, DWORD base, unsigned int length);
	DWORD DerefOffsets(_In_ HANDLE process, DWORD base);

	static DWORD GetThreadAddress(_In_ HANDLE process, DWORD pId, unsigned int threadId);
};

