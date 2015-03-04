#include "PointerPath.h"
#include <tchar.h>
#include "Log.h"

#ifdef _WIN64
#define LDT_ENTRY32 WOW64_LDT_ENTRY
#define GetThreadSelectorEntry32 Wow64GetThreadSelectorEntry
#else
#define LDT_ENTRY32 LDT_ENTRY
#define GetThreadSelectorEntry32 GetThreadSelectorEntry
#endif



PointerPath32::PointerPath32(std::vector<int> offsets) : offsets(offsets)
{
}


PointerPath32::~PointerPath32()
{
}


_Success_(return) bool PointerPath32::GetModuleInfo(DWORD pId, _In_ TCHAR * moduleName, _Out_ MODULEENTRY32 &moduleInfo)
{
	bool found = false;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pId);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		Log("ERROR | PointerPath::GetModuleInfo | Couldn't create module snapshot!");
		LogLastError();
	}
	else 
	{
		MODULEENTRY32 entry;
		entry.dwSize = sizeof(MODULEENTRY32);

		if (!Module32First(snapshot, &entry))
		{
			Log("ERROR | PointerPath::GetModuleInfo | Couldn't enumerate first module!");
			LogLastError();
		}
		else
		{
			do
			{
				if (_tcsicmp(entry.szModule, moduleName) == 0)
				{
					Log("INFO  | PointerPath::GetModuleInfo | Found module", moduleName);
					moduleInfo = entry;
					found = true;
					break;
				}
			} while (Module32Next(snapshot, &entry));
		}
		CloseHandle(snapshot);
	}

	return found;
}


DWORD PointerPath32::GetStackStart(_In_ HANDLE process, DWORD pId, _In_ HANDLE thread)
{
	// address of the top of the stack
	DWORD stacktopAddress = 0;

	SIZE_T read = 0;

	CONTEXT context;
	ZeroMemory(&context, sizeof(context));
	context.ContextFlags = CONTEXT_SEGMENTS;
	if (GetThreadContext(thread, &context))
	{
		LDT_ENTRY32 ldtEntry;
		if (GetThreadSelectorEntry32(thread, context.SegFs, &ldtEntry))
		{
			DWORD stackInfo = ldtEntry.BaseLow + (ldtEntry.HighWord.Bytes.BaseMid << 16) +
							  (ldtEntry.HighWord.Bytes.BaseHi << 24);
			if (!ReadProcessMemory(process, reinterpret_cast<void*>(stackInfo + sizeof(DWORD)), &stacktopAddress, 
				sizeof(DWORD), &read))
			{
				Log("ERROR | PointerPath::GetStackStart | Couldn't read address of the top of the stack!");
				LogLastError();
				return 0;
			}
			else if (read != sizeof(DWORD))
			{
				Log("WARNING | PointerPath::GetStackStart | Didn't finish while reading address of the top of the stack!");
			}
		}
		else
		{
			Log("ERROR | PointerPath::GetStackStart | Could not get descriptor table entry!");
			LogLastError();
			return 0;
		}
	}
	else
	{
		Log("ERROR | PointerPath::GetStackStart | Could not get thread context!");
		LogLastError();
		return 0;
	}

	// with stacktopAddress, we know where the stack ends. Now search the last SEARCH_SIZE bytes of the
	// stack for a pointer, that points to the function that calls "ExitXXXXXThread"
	// According to CheatEngine, it's the first entry that points to an address in kernel32

	// read last SEARCH_SIZE bytes of the stack
	unsigned int const SEARCH_SIZE = 4096;
	unsigned int const LENGTH = SEARCH_SIZE / sizeof(DWORD);
	DWORD stack[LENGTH];
	DWORD firstAddress = stacktopAddress - SEARCH_SIZE;
	if (!ReadProcessMemory(process, reinterpret_cast<void *>(firstAddress), stack, SEARCH_SIZE, &read))
	{
		Log("ERROR | PointerPath::GetStackStart | Couldn't read part of the stack!");
		LogLastError();
		return 0;
	}
	else if (read != SEARCH_SIZE)
	{
		Log("WARNING | PointerPath::GetStackStart | Didn't finish while reading part of the stack");
	}

	// get start and end address of kernel32 module
	MODULEENTRY32 moduleInfo;
	if (!PointerPath32::GetModuleInfo(pId, TEXT("kernel32.dll"), moduleInfo))
	{
		Log("ERROR | PointerPath::GetStackStart | Couldn't get kernel32.dll module info!");
		return 0;
	}
	UINT_PTR kernel32Start = reinterpret_cast<UINT_PTR>(moduleInfo.hModule);
	UINT_PTR kernel32End = kernel32Start + moduleInfo.modBaseSize;
	
	for (int i = LENGTH - 1; i >= 0; --i)
	{
		if (stack[i] >= kernel32Start && stack[i] < kernel32End)
		{
			// Hoorayyy, finally found it!
			DWORD stackStart = firstAddress + i * sizeof(DWORD);
			Log("INFO  | PointerPath::GetStackStart | Found stack start: ", stackStart, true);
			return stackStart;
		}
	}

	Log("WARNING | PointerPath::GetStackStart | Couldn't find stack start!");
	return 0;
}


DWORD PointerPath32::GetThreadAddress(_In_ HANDLE process, DWORD pId, unsigned int threadId)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		Log("ERROR | PointerPath::GetThreadAddress | Couldn't create thread snapshot!");
		LogLastError();
		return 0;
	}
	
	DWORD address = 0;
	THREADENTRY32 entry;
	entry.dwSize = sizeof(THREADENTRY32);

	if (Thread32First(snapshot, &entry))
	{
		unsigned int currentId = 0;
		do
		{
			if (entry.th32OwnerProcessID == pId)
			{
				if (currentId == threadId)
				{
					HANDLE thread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_GET_CONTEXT, FALSE, entry.th32ThreadID);
					if (thread == nullptr)
					{
						Log("ERROR | PointerPath::GetThreadAddress | Couldn't open thread with ID ", entry.th32ThreadID);
						LogLastError();
					}
					else
					{
						address = GetStackStart(process, pId, thread);
						CloseHandle(thread);
					}
					break;
				}
				currentId += 1;
			}
		} while (Thread32Next(snapshot, &entry));
	}
	else
	{
		Log("ERROR | PointerPath::GetThreadAddress | Couldn't enumerate first thread!");
		LogLastError();
	}

	CloseHandle(snapshot);
	return address;
}


_Success_(return) bool PointerPath32::Deref(_In_ HANDLE process, DWORD base, _Out_ long &value)
{
	DWORD pointer = DerefOffsets(process, base);
	SIZE_T read = 0;

	if (!ReadProcessMemory(process, reinterpret_cast<void *>(pointer), &value, sizeof(value), &read))
	{
		return false;
	}
	else if (read != sizeof(value))
	{
		Log("WARNING | PointerPath::Deref | Didn't finish while reading int value at", pointer, true);
	}

	return true;
}


std::string PointerPath32::Deref(_In_ HANDLE process, DWORD base, unsigned int const LENGTH)
{
	DWORD pointer = DerefOffsets(process, base);
	SIZE_T read = 0;

	char * buffer = new char[LENGTH + 1];
	if (!ReadProcessMemory(process, reinterpret_cast<void *>(pointer), buffer, LENGTH, &read))
	{
		buffer[0] = '\0';
	}
	else if (read != LENGTH)
	{
		Log("WARNING | PointerPath::Deref | Didn't finish while reading string at ", pointer, true);
	}

	// make sure the string terminates
	buffer[LENGTH] = '\0';

	std::string str = std::string(buffer);
	delete[] buffer;
	return str;
}


DWORD PointerPath32::DerefOffsets(_In_ HANDLE process, DWORD base)
{
	DWORD pointer = base;
	SIZE_T read = 0;

	for (auto it = offsets.begin(); it != offsets.end() - 1; it += 1)
	{
		if (!ReadProcessMemory(process, reinterpret_cast<void *>(pointer + *it), &pointer, sizeof(DWORD), &read))
		{
			return 0;
		}
		else if (read != sizeof(DWORD))
		{
			Log("WARNING | PointerPath::DerefOffsets | Didn't finish while derefing ", pointer + *it, true);
		}
	}

	return pointer + offsets[offsets.size() - 1];
}