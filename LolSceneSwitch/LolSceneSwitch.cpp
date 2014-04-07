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
		return OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
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
		return INVALID_HANDLE_VALUE;
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
			_tcscmp(ffd.cFileName + nameLength - 4, TEXT(".txt")) == 0 && 
			CompareFileTime(&ffd.ftCreationTime, creationTime) == 1)
		{
			StringCchCopy(newestLog, MAX_PATH, ffd.cFileName);
			*creationTime = ffd.ftCreationTime;
		}
	} 
	while (FindNextFile(searchHandle, &ffd) != 0);
	FindClose(searchHandle);

	TCHAR path[MAX_PATH];
	if (FAILED(StringCchLength(newestLog, MAX_PATH, &nameLength)) || pathLength + nameLength > MAX_PATH)
	{
		return INVALID_HANDLE_VALUE;
	}
	StringCchCopy(path, MAX_PATH, directory);
	StringCchCat(path, MAX_PATH, TEXT("\\"));
	StringCchCat(path, MAX_PATH, newestLog);

	Log("GetNewestLog() - found newest log file at ", path);
	
	return CreateFile(path,
					  GENERIC_READ, // we only need to read
					  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // let LoL do whatever it pleases (Mundo!)
					  nullptr,
					  OPEN_EXISTING,
					  FILE_ATTRIBUTE_NORMAL,
					  nullptr);
}

HWND GetLolWindow(DWORD pid, unsigned int intervall)
{
	HWND hwndCurrent = GetWindow(GetDesktopWindow(), GW_CHILD);
	DWORD pidCurrent = 0;

	do
	{
		GetWindowThreadProcessId(hwndCurrent, &pidCurrent);
		// checking for GetWindowTextLength to not equal 0 is to ensure that we found 
		// the actual game window, not the small icon that pops up just before the game starts
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
	if (settings.enabled && lolProcessMonitor == nullptr)
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
		if (currentGameScene != OBSGetSceneName())
		{
			OBSSetScene(currentGameScene, true);
			Log("ChangeScene() - changed scene to ", currentGameScene);
		}
		else
		{
			Log("ChangeScene() - scene was already correct");
		}
	}
	else
	{
		// this should not happen anymore!!! if it happens, ChangeScene 
		// is called by WindowMonitorThread but LogMonitorThread hasn't
		// read anything in the log that lead to call SetCurrentScene
		// which probably means that riot messed with the log files
		Log("ChangeScene() - currentGameScene was null!!!");
	}
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
		// no tabbedoutScene means no WindowMonitorThread, therefore we have
		// to chage the scene at this point
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
			Sleep(intervall);
		}
		if (!instance->runMonitoring)
		{ 
			return 1;
		}
		Log("ProcessMonitorThread() - League of Legends process started");

		// get start time to check for correct log file
		FILETIME temp;
		GetProcessTimes(process, &instance->lolStartTime, &temp, &temp, &temp);
		
		// start log monitoring
		instance->lolProcessClosed = false;
		instance->stateChanged = false;
		HANDLE logMonitor = nullptr;
		if ((logMonitor = CreateThread(nullptr, 0, LogMonitorThread, instance, 0, nullptr)) != nullptr)
		{
			// wait for league process to end
			HANDLE waitObjects[2] = { process, instance->eventEndMonitoring };
			WaitForMultipleObjects(2, waitObjects, FALSE, INFINITE);
			if (instance->runMonitoring)
			{
				Log("ProcessMonitorThread() - League of Legends process closed");
			}
			else
			{
				Log("ProcessMonitorThread() - event end monitoring sent");
			}

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
		if (file == INVALID_HANDLE_VALUE)
		{
			Log("LogMonitorThread() - Couldn't open log file");
			return 0;
		}
		else if (file == nullptr)
		{
			Log("LogMonitorThread() - Couldn't find any log file, trying again");
		}
		else
		{
			if (CompareFileTime(&creationTime, &instance->lolStartTime) >= 0)
			{
				// log file was created after the process started, nice
				break;
			}
			else
			{
				Log("LogMonitorThread() - found log file was to old, waiting for the current log to be created...");
			}
		}

		Sleep(intervall);
	}
	
	HANDLE windowMonitor = nullptr;

	char testRead;
	DWORD bytesRead;
	if (ReadFile(file, &testRead, 1, &bytesRead, nullptr))
	{
		Log("LogMonitorThread() - started reading from log");

		char const LOAD_STRING[] = "Set focus to app";
		char const MAP_STRING[] = "Initializing GameModeComponents for mode=";
		char const START_STRING[] = "HUDProcess";
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
				instance->SetCurrentScene(settings.loadscreenScene[SINGLE]);
			}
		}

		unsigned int mapIndex = SINGLE;
		if ((settings.loadscreenMaps || settings.gameMaps || settings.endgameMaps) &&
			instance->WaitForStringInLog(MAP_STRING, file))
		{
			char buffer[5];
			while (!instance->lolProcessClosed && (!ReadFile(file, buffer, 4, &bytesRead, nullptr) || bytesRead == 0))
			{
				Sleep(intervall);
			}
			buffer[bytesRead] = '\0';

			if (strcmp(buffer, "CLAS") == 0)
			{
				mapIndex = SUMMONERS_RIFT;
				Log("LogMonitorThread() - recognized Summoners Rift");
			}
			else if (strcmp(buffer, "ODIN") == 0)
			{
				mapIndex = CRYSTAL_SCAR;
				Log("LogMonitorThread() - recognized Crystal Scar");
			}
			else if (strcmp(buffer, "ARAM") == 0)
			{
				mapIndex = HOWLING_ABYSS;
				Log("LogMonitorThread() - recognized Howling Abyss");
			}
			else
			{
				mapIndex = SINGLE;
				Log("LogMonitorThread() - Invalid map!");
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

	HWND window = GetLolWindow(instance->lolPid, intervall);
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