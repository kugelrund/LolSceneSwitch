#include "LolSceneSwitch.h"
#include <tlhelp32.h>
#include <tchar.h>
#include <strsafe.h>
#include "LolSceneSwitchDialog.h"
#include "Log.h"

LolSceneSwitch * plugin = nullptr;
HINSTANCE hInstDLL = nullptr;

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hInstDLL = hinstDLL;
	return TRUE;
}

CTSTR GetPluginName()
{
	return TEXT("League of Legends Scene Switch");
}

CTSTR GetPluginDescription()
{
	return TEXT("Switches scenes according do the state of League of Legends.");
}

bool LoadPlugin()
{
	if (plugin != nullptr)
	{
		return false;
	}

	InitLog(OBSGetPluginDataPath());
	plugin = new LolSceneSwitch();
	return true;
}

void UnloadPlugin()
{
	if (plugin == nullptr)
	{
		return;
	}

	delete plugin;
	plugin = nullptr;
	CloseLog();
}

void OnStartStream()
{
	if (plugin != nullptr)
	{
		plugin->StartMonitoring();
	}
}

void OnStopStream()
{
	if (plugin != nullptr)
	{
		plugin->EndMonitoring();
	}
}

void ConfigPlugin(HWND hWnd)
{
	if (plugin != nullptr)
	{
		bool restart = false;
		if (plugin->IsMonitoring())
		{
			restart = true;
			plugin->EndMonitoring();
		}

		LolSceneSwitchDialog dialog(plugin->GetSettingsRef());
		dialog.Show(hWnd, hInstDLL);

		if (restart)
		{
			plugin->StartMonitoring();
		}
	}
}





HANDLE GetProcessByName(TCHAR const * name, DWORD & pid)
{
	pid = 0;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_tcscmp(entry.szExeFile, name) == 0)
			{
				pid = entry.th32ProcessID;
				break;
			}
		}
		CloseHandle(snapshot);
	}

	if (pid != 0)
	{
		Log("GetProcessByName() - found LoL process with pid ", pid);
		return OpenProcess(SYNCHRONIZE, FALSE, pid);
	}
	else
	{
		return nullptr;
	}
}

HANDLE GetNewestLog(TCHAR const * directory, LPFILETIME const creationTime)
{
	creationTime->dwHighDateTime = 0;
	creationTime->dwLowDateTime = 0;

	TCHAR dirPath[MAX_PATH];
	size_t pathLength;
	if (FAILED(StringCchLength(directory, MAX_PATH, &pathLength)) || pathLength + 3 > MAX_PATH)
	{
		return nullptr;
	}

	StringCchCopy(dirPath, MAX_PATH, directory);
	StringCchCat(dirPath, MAX_PATH, TEXT("\\*"));

	TCHAR newestLog[MAX_PATH];
	newestLog[0] = '\0';
	WIN32_FIND_DATA ffd;
	size_t nameLength;

	HANDLE searchHandle = FindFirstFile(dirPath, &ffd);
	if (searchHandle == INVALID_HANDLE_VALUE)
	{
		return nullptr;
	}
	do
	{
		if (SUCCEEDED(StringCchLength(ffd.cFileName, MAX_PATH, &nameLength)) && 
			_tcscmp(ffd.cFileName + nameLength - 4, TEXT(".txt")) == 0 && CompareFileTime(&ffd.ftCreationTime, creationTime) == 1)
		{
			StringCchCopy(newestLog, MAX_PATH, ffd.cFileName);
			*creationTime = ffd.ftCreationTime;
		}
	} 
	while (FindNextFile(searchHandle, &ffd) != 0);
	FindClose(searchHandle);

	TCHAR path[MAX_PATH];
	if (pathLength + nameLength > MAX_PATH)
	{
		return nullptr;
	}
	StringCchCopy(path, MAX_PATH, directory);
	StringCchCat(path, MAX_PATH, TEXT("\\"));
	StringCchCat(path, MAX_PATH, newestLog);

	Log("GetNewestLog() - found newest log file at ", path);
	
	return CreateFile(path,
					  GENERIC_READ | GENERIC_WRITE,
					  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					  nullptr,
					  OPEN_EXISTING,
					  FILE_ATTRIBUTE_NORMAL,
					  nullptr);
}

HWND GetLoLWindow(DWORD pid, unsigned int intervall)
{
	HWND hwndCurrent = GetWindow(GetDesktopWindow(), GW_CHILD);
	DWORD pidCurrent = 0;

	do
	{
		GetWindowThreadProcessId(hwndCurrent, &pidCurrent);
		if (pidCurrent == pid && IsWindowVisible(hwndCurrent) && GetWindowTextLength(hwndCurrent) != 0)
		{
			Log("GetLoLWindow() - found LoL window");
			return hwndCurrent;
		}
	} 
	while (hwndCurrent = GetNextWindow(hwndCurrent, GW_HWNDNEXT));
	
	Log("GetLoLWindow() - no window found");
	return nullptr;
}

bool HasFocus(HWND window)
{
	DWORD pid1 = 0;
	DWORD pid2 = 0;
	GetWindowThreadProcessId(window, &pid1);
	GetWindowThreadProcessId(GetForegroundWindow(), &pid2);
	return pid1 == pid2 && pid1 != 0 && pid2 != 0;
}





LolSceneSwitch::LolSceneSwitch() : settings(),
								   lolProcessMonitor(nullptr),
								   eventEndMonitoring(CreateEvent(nullptr, TRUE, FALSE, TEXT("endMonitoring"))),
								   runMonitoring(false),
								   lolProcessClosed(false),
								   lolStartTime(),
								   lolPid(0),
								   currentGameScene(),
								   stateChanged(false)
{
}

LolSceneSwitch::~LolSceneSwitch()
{
	CloseHandle(eventEndMonitoring);
}

bool LolSceneSwitch::IsMonitoring() const
{
	return runMonitoring;
}

LolSceneSwitchSettings & LolSceneSwitch::GetSettingsRef()
{
	return settings;
}

void LolSceneSwitch::StartMonitoring()
{
	if (settings.enabled && (!settings.loadscreenScene[SINGLE].IsEmpty() || 
							 !settings.gameScene[SINGLE].IsEmpty() || 
							 !settings.endgameScene[SINGLE].IsEmpty()) && lolProcessMonitor == nullptr)
	{
		runMonitoring = true;
		lolProcessMonitor = CreateThread(nullptr, 0, ProcessMonitorThread, this, 0, nullptr);
	}
	Log("StartMonitoring()");
}

void LolSceneSwitch::EndMonitoring()
{
	runMonitoring = false;
	if (lolProcessMonitor != nullptr)
	{
		SetEvent(eventEndMonitoring);
		WaitForSingleObject(lolProcessMonitor, INFINITE);
		CloseHandle(lolProcessMonitor);
		lolProcessMonitor = nullptr;
		ResetEvent(eventEndMonitoring);
	}
	Log("EndMonitoring()");
}

void LolSceneSwitch::ChangeScene() const
{
	if (currentGameScene != nullptr)
	{
		OBSSetScene(currentGameScene, true);
	}
	Log("ChangeScene() - changed scene to ", currentGameScene);
}

void LolSceneSwitch::RevertScene() const
{
	if (lolProcessClosed)
	{
		OBSSetScene(settings.closedScene, true);
		Log("RevertScene() - set scene to closed = ", settings.closedScene);
	}
	else
	{
		OBSSetScene(settings.tabbedoutScene, true);
		Log("RevertScene() - set scene to tabbed out = ", settings.tabbedoutScene);
	}
}

void LolSceneSwitch::SetCurrentScene(String const & scene)
{
	currentGameScene = scene;
	stateChanged = true;
	if (settings.tabbedoutScene.IsEmpty())
	{
		ChangeScene();
	}
	Log("SetCurrentScene() - set scene to ", scene);
}

DWORD WINAPI LolSceneSwitch::ProcessMonitorThread(LPVOID lpParam)
{
	LolSceneSwitch * instance = static_cast<LolSceneSwitch *>(lpParam);
	unsigned int const intervall = instance->settings.intervall;

	while (instance->runMonitoring)
	{
		// wait for leagueoflegends process to start
		HANDLE process = nullptr;
		while (((process = GetProcessByName(TEXT("League of Legends.exe"), instance->lolPid)) == nullptr) && instance->runMonitoring)
		{
			GetSystemTimeAsFileTime(&instance->lolStartTime);
			Sleep(intervall);
		}
		if (!instance->runMonitoring)
		{ 
			return 1;
		}
		Log("ProcessMonitorThread() - League of Legends process started");
		
		// start log monitoring
		instance->lolProcessClosed = false;
		instance->stateChanged = false;
		HANDLE logMonitor = nullptr;
		if ((logMonitor = CreateThread(nullptr, 0, LogMonitorThread, instance, 0, nullptr)) != nullptr)
		{
			// wait for league process to end
			HANDLE waitObjects[2] = { process, instance->eventEndMonitoring };
			WaitForMultipleObjects(2, waitObjects, FALSE, INFINITE);
			Log("ProcessMonitorThread() - League of Legends process closed");

			// league process ended, shutting down monitoring threads
			instance->lolProcessClosed = true;
			WaitForSingleObject(logMonitor, INFINITE);
			CloseHandle(logMonitor);
		}
		CloseHandle(process);

		if (!instance->settings.closedScene.IsEmpty() && instance->runMonitoring)
		{
			instance->RevertScene();
		}
	}

	return 1;
}

template <unsigned int N> bool LolSceneSwitch::WaitForStringInLog(char const (&searchString)[N], HANDLE file) const
{
	unsigned int const intervall = settings.intervall;
	unsigned int const BUFFER_SIZE = 256;
	unsigned int const OVERLAP_SIZE = N - 2;

	char overlap[BUFFER_SIZE + OVERLAP_SIZE];
	char * const buffer = overlap + OVERLAP_SIZE;
	char * found = nullptr;
	DWORD bytesRead = 0;
			
	while (!lolProcessClosed)
	{
		if (ReadFile(file, buffer, BUFFER_SIZE - 1, &bytesRead, nullptr) && bytesRead)
		{
			buffer[bytesRead] = '\0';
			if ((found = strstr(overlap, searchString)) != nullptr)
			{
				SetFilePointer(file, static_cast<unsigned int>(found - (buffer + bytesRead)) + (N - 1), nullptr, FILE_CURRENT);
				return true;
			}
			memcpy(overlap, overlap + bytesRead, OVERLAP_SIZE);
		}
		else
		{
			Sleep(intervall);
		}
	}

	return false;
}

DWORD WINAPI LolSceneSwitch::LogMonitorThread(LPVOID lpParam)
{
	LolSceneSwitch * instance = static_cast<LolSceneSwitch *>(lpParam);
	LolSceneSwitchSettings const & settings = instance->settings;
	unsigned int const intervall = settings.intervall;

	HANDLE file = INVALID_HANDLE_VALUE;
	FILETIME creationTime;
	while (!instance->lolProcessClosed)
	{
		file = GetNewestLog(settings.lolPath + TEXT("\\Logs\\Game - R3d Logs"), &creationTime);
		if (file == nullptr || file == INVALID_HANDLE_VALUE)
		{
			Log("LogMonitorThread() - Couldn't open log file");
			return 0;
		}

		if (CompareFileTime(&creationTime, &instance->lolStartTime) >= 0)
		{
			break;
		}

		Log("LogMonitorThread() - found log file was to old, waiting for the current log to be created...");
		Sleep(intervall);
	}
	
	HANDLE windowMonitor = nullptr;
	if (ReadFile(file, nullptr, 0, nullptr, nullptr))
	{
		Log("LogMonitorThread() - started reading from log");

		char const LOAD_STRING[] = "Set focus to app";
		char const MAP_STRING[] = "Adding level zip file: Map";
		char const START_STRING[] = "DrawSysInfo";
		char const END_STRING[] = "End game message processing!";

		if ((!settings.tabbedoutScene.IsEmpty() || !settings.loadscreenScene[SINGLE].IsEmpty()) &&
			instance->WaitForStringInLog(LOAD_STRING, file))
		{
			if (!settings.tabbedoutScene.IsEmpty())
			{
				windowMonitor = CreateThread(nullptr, 0, WindowFocusMonitorThread, instance, 0, nullptr);
			}
			if (!settings.loadscreenScene[SINGLE].IsEmpty() && !settings.loadscreenMaps)
			{
				instance->SetCurrentScene(settings.loadscreenScene[0]);
			}
		}

		unsigned int mapIndex = SINGLE;
		if ((settings.loadscreenMaps || settings.gameMaps || settings.endgameMaps) &&
			instance->WaitForStringInLog(MAP_STRING, file))
		{
			DWORD bytesRead;
			char buffer[3];
			while (!instance->lolProcessClosed && (!ReadFile(file, buffer, 2, &bytesRead, nullptr) || bytesRead == 0))
			{
				Sleep(intervall);
			}
			buffer[bytesRead] = '\0';
			if (buffer[1] == '.')
			{
				buffer[1] = '\0';
			}

			unsigned int const mapNumber = atoi(buffer);
			Log("LogMonitorThread() - mapNumber: ", mapNumber);
			switch (mapNumber)
			{
				case 1:
					mapIndex = SUMMONERS_RIFT;
					break;
				case 8:
					mapIndex = CRYSTAL_SCAR;
					break;
				case 10:
					mapIndex = TWISTED_TREELINE;
					break;
				case 12:
					mapIndex = HOWLING_ABYSS;
					break;
				default:
					mapIndex = SINGLE;
					Log("LogMonitorThread() - Invalid map number!");
			}

			if (!settings.loadscreenScene[mapIndex].IsEmpty() && settings.loadscreenMaps)
			{
				instance->SetCurrentScene(settings.loadscreenScene[mapIndex]);
			}
		}
						
		String const & gameScene = settings.gameScene[settings.gameMaps ? mapIndex : SINGLE];
		if (!gameScene.IsEmpty() && instance->WaitForStringInLog(START_STRING, file))
		{
			instance->SetCurrentScene(gameScene);
		}

		String const & endgameScene = settings.endgameScene[settings.endgameMaps ? mapIndex : SINGLE];
		if (!endgameScene.IsEmpty() && instance->WaitForStringInLog(END_STRING, file))
		{
			instance->SetCurrentScene(endgameScene);
		}
	}
	else
	{
		Log("LogMonitorThread() - couldn't read log");
	}

	if (windowMonitor != nullptr)
	{
		WaitForSingleObject(windowMonitor, INFINITE);
		CloseHandle(windowMonitor);
	}
	CloseHandle(file);

	return 1;
}

DWORD WINAPI LolSceneSwitch::WindowFocusMonitorThread(LPVOID lpParam)
{
	LolSceneSwitch * instance = static_cast<LolSceneSwitch *>(lpParam);
	unsigned int const intervall = instance->settings.intervall;

	HWND window = GetLoLWindow(instance->lolPid, intervall);
	bool focus = true;

	while (!instance->lolProcessClosed)
	{
		if (focus != HasFocus(window))
		{
			if (focus)
			{
				Log("WindowFocusMonitorThread() - lost focus");
				instance->RevertScene();
				focus = false;
			}
			else
			{
				Log("WindowFocusMonitorThread() - gained focus");
				instance->ChangeScene();
				focus = true;
			}
		}

		if (instance->stateChanged && focus)
		{
			Log("WindowFocusMonitorThread() - game state changed");
			if (_tcscmp(instance->currentGameScene, OBSGetSceneName()) != 0)
			{
				instance->ChangeScene();
			}
			instance->stateChanged = false;
		}

		Sleep(intervall);
	}

	return 1;
}