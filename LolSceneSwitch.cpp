#include "LolSceneSwitch.h"
#include <tchar.h>
#include <strsafe.h>
#include "SettingsDialog.h"
#include "PointerPath.h"
#include "Log.h"



LolSceneSwitch *plugin = nullptr;
HINSTANCE hInstDLL = nullptr;


BOOL CALLBACK DllMain(_In_ HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
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
	Log("INFO  | Loaded LolSceneSwitch v0.2 Alpha 8");
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


void ConfigPlugin(_In_ HWND hWnd)
{
	if (plugin != nullptr)
	{
		bool restart = false;
		if (plugin->IsMonitoring())
		{
			restart = true;
			plugin->EndMonitoring();
		}

		SettingsDialog dialog(plugin->GetSettingsRef());
		dialog.Show(hWnd, hInstDLL);

		if (restart)
		{
			plugin->StartMonitoring();
		}
	}
}




HANDLE GetProcessByName(_In_ TCHAR const *name, _Out_ DWORD &pid)
{
	pid = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		Log("ERROR | GetProcessByName | Couldn't create process snapshot!");
		LogLastError();
		return nullptr;
	}

	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(snapshot, &entry))
	{
		Log("ERROR | GetProcessByName | Couldn't enumerate first process!");
		LogLastError();
	}
	else
	{
		while (Process32Next(snapshot, &entry))
		{
			if (_tcscmp(entry.szExeFile, name) == 0)
			{
				pid = entry.th32ProcessID;
				break;
			}
		}
	}
	CloseHandle(snapshot);

	if (pid != 0)
	{
		Log("INFO  | GetProcessByName | Found process", name);
		HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (process == nullptr)
		{
			Log("ERROR | GetProcessByName | Couldn't open process", name);
			LogLastError();
			return nullptr;
		}
		else
		{
			return process;
		}
	}
	else
	{
		return nullptr;
	}
}


HANDLE GetNewestLog(_In_ TCHAR const * directory, _Out_ LPFILETIME const creationTime)
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
		Log("ERROR | GetNewestLog | Couldn't enumerate files in", dirPath);
		LogLastError();
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

	Log("INFO  | GetNewestLog | Found newest log file at", path);
	
	return CreateFile(path,
					  GENERIC_READ, // we only need to read
					  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // let LoL do whatever it pleases (Mundo!)
					  nullptr,
					  OPEN_EXISTING,
					  FILE_ATTRIBUTE_NORMAL,
					  nullptr);
}


HANDLE GetLogFile(String path, _In_ FILETIME * const lolStartTime)
{
	FILETIME creationTime;
	HANDLE file = GetNewestLog(path + TEXT("\\Logs\\Game - R3d Logs"), &creationTime);
	if (file == INVALID_HANDLE_VALUE)
	{
		Log("ERROR | GetLogReader | Couldn't open log file!");
		LogLastError();
	}
	else if (file == nullptr)
	{
		Log("INFO  | GetLogReader | Couldn't find any log file, trying again");
	}
	else
	{
		if (CompareFileTime(&creationTime, lolStartTime) >= 0)
		{
			return file;
		}
		else
		{
			Log("INFO  | GetLogReader | Found log file was to old, waiting for the current log to be created");
		}
	}

	return nullptr;
}


HWND GetWindowById(DWORD pId)
{
	HWND hwndCurrent = GetWindow(GetDesktopWindow(), GW_CHILD);
	if (hwndCurrent == nullptr)
	{
		Log("ERROR | GetWindowById | Couldn't get desktop window handle!");
		LogLastError();
		return nullptr;
	}

	DWORD pIdCurrent = 0;
	do
	{
		GetWindowThreadProcessId(hwndCurrent, &pIdCurrent);
		// checking for GetWindowTextLength to not equal 0 is to ensure that we found 
		// the actual game window, not the small icon that pops up just before the game starts
		if (pIdCurrent == pId && IsWindowVisible(hwndCurrent) && GetWindowTextLength(hwndCurrent) != 0)
		{
			Log("INFO  | GetWindowById | Found window with pId", pId);
			return hwndCurrent;
		}
	} 
	while (hwndCurrent = GetNextWindow(hwndCurrent, GW_HWNDNEXT));
	return nullptr;
}


bool HasFocus(_In_ HWND window)
{
	DWORD pid1 = 0;
	DWORD pid2 = 0;
	GetWindowThreadProcessId(window, &pid1);
	GetWindowThreadProcessId(GetForegroundWindow(), &pid2);
	return pid1 == pid2 && pid1 != 0 && pid2 != 0;
}




LolSceneSwitch::LolSceneSwitch() : settings(),
								   lolProcessMonitor(nullptr),
								   runMonitoring(false),
								   currentMap(Map::UNKNOWN)
{
}


LolSceneSwitch::~LolSceneSwitch()
{
}


bool LolSceneSwitch::IsMonitoring() const
{
	return runMonitoring;
}


Settings & LolSceneSwitch::GetSettingsRef()
{
	return settings;
}


void LolSceneSwitch::StartMonitoring()
{
	if (settings.enabled && lolProcessMonitor == nullptr)
	{
		runMonitoring = true;
		lolProcessMonitor = CreateThread(nullptr, 0, MonitorThread, this, 0, nullptr);
	}
	Log("INFO  | LolSceneSwitch::StartMonitoring");
}


void LolSceneSwitch::EndMonitoring()
{
	runMonitoring = false;
	if (lolProcessMonitor != nullptr)
	{
		WaitForSingleObject(lolProcessMonitor, INFINITE);
		CloseHandle(lolProcessMonitor);
		lolProcessMonitor = nullptr;
	}
	Log("INFO  | LolSceneSwitch::EndMonitoring");
}


void LolSceneSwitch::SetScene(String scene) const
{
	if (!scene.IsEmpty() && scene != OBSGetSceneName())
	{
		OBSSetScene(scene, true);
	}
}


void LolSceneSwitch::ChangeScene(State state)
{
	StateScenes scene = settings.scenes[state];
	if (scene.useMaps)
	{
		switch (currentMap)
		{
			case Map::SUMMONERS_RIFT:
				SetScene(scene.mapScenes.summonersRift);
				break;
			case Map::CRYSTAL_SCAR:
				SetScene(scene.mapScenes.crystalScar);
				break;
			case Map::TWISTED_TREELINE:
				SetScene(scene.mapScenes.twistedTreeline);
				break;
			case Map::HOWLING_ABYSS:
				SetScene(scene.mapScenes.howlingAbyss);
				break;
		}
	}
	else
	{
		SetScene(scene.single);
	}
}


DWORD WINAPI LolSceneSwitch::MonitorThread(_In_ LPVOID lpParam)
{
	LolSceneSwitch * instance = static_cast<LolSceneSwitch *>(lpParam);
	unsigned int const INTERVALL = instance->settings.intervall;

	// Handles etc. for the client
	HANDLE clientProcess = nullptr;
	HWND clientWindow = nullptr;
	DWORD clientPid = 0;
	// Handles etc. for the game
	HANDLE gameProcess = nullptr;
	HWND gameWindow = nullptr;
	DWORD gamePid;
	FILETIME gameStartTime;
	// temp var for saving the exit code
	DWORD exitCode;

	// Used to read the log file
	LogReader * reader = nullptr;

	// Stuff for reading memory
	PointerPath32 clienStatePointer({ -0x658, 0x78, 0x614, 0x4, 0x4C8 });
	PointerPath32 map1Pointer({ -0x780, 0x20 });
	PointerPath32 map2Pointer({ -0x784, 0x20 });
	DWORD clientStateAddress = 0;
	long clientState = 0;
	DWORD mapAddress = 0;

	// info variables of the state of LoL
	bool ingame = false;
	bool postGame = false;
	State state = State::CLIENTOUT;
	State oldState = State::CLIENTOUT;
	bool newMapInfo = false;

	// The main loop for monitoring LoL
	while (instance->runMonitoring)
	{
		// Try to get handles for the client and game process
		if (clientProcess == nullptr)
		{
			clientProcess = GetProcessByName(TEXT("LolClient.exe"), clientPid);
		}
		if (gameProcess == nullptr)
		{
			gameProcess = GetProcessByName(TEXT("League of Legends.exe"), gamePid);
			if (gameProcess != nullptr)
			{
				ingame = true; 
				FILETIME temp;
				GetProcessTimes(gameProcess, &gameStartTime, &temp, &temp, &temp);
			}
		}

		// We have a client handle
		if (clientProcess != nullptr)
		{
			// check if it's still running
			if (GetExitCodeProcess(clientProcess, &exitCode) && exitCode == STILL_ACTIVE)
			{
				if (clientWindow == nullptr)
				{
					clientWindow = GetWindowById(clientPid);
				}
				if (clientStateAddress == 0)
				{
					clientStateAddress = PointerPath32::GetThreadAddress(clientProcess, clientPid, 0);
				}

				if (!ingame)
				{
					if (instance->settings.scenes[State::CLIENTOUT].single.IsEmpty() ||
						(clientWindow != nullptr && HasFocus(clientWindow)))
					{
						if (clienStatePointer.Deref(clientProcess, clientStateAddress, clientState) && clientState == 1)
						{
							if (postGame)
							{
								state = State::POSTGAME;
							}
							else
							{
								state = State::CHAMPSELECT;
							}
						}
						else
						{
							postGame = false;
							state = State::CLIENT;
						}
					}
					else
					{
						state = State::CLIENTOUT;
					}
				}
			}
			else
			{
				Log("INFO  | LolSceneSwitch::MonitorThread | LoL client process exited!");
				CloseHandle(clientProcess);
				clientProcess = nullptr;
				clientWindow = nullptr;
				clientStateAddress = 0;
			}
		}

		// we have a game handle
		if (gameProcess != nullptr)
		{
			// check if it's still running
			if (GetExitCodeProcess(gameProcess, &exitCode) && exitCode == STILL_ACTIVE)
			{
				if (gameWindow == nullptr)
				{
					gameWindow = GetWindowById(gamePid);
				}
				if (reader == nullptr)
				{
					HANDLE file = GetLogFile(instance->settings.lolPath, &gameStartTime);
					if (file != nullptr)
					{
						reader = &LogReader(file);
					}
				}
				if (mapAddress == 0)
				{
					mapAddress = PointerPath32::GetThreadAddress(gameProcess, gamePid, 0);
				}

				if (instance->currentMap == Map::UNKNOWN)
				{
					std::string mapString1 = map1Pointer.Deref(gameProcess, mapAddress, 5);
					std::string mapString2 = map2Pointer.Deref(gameProcess, mapAddress, 5);
					if (mapString1.compare("Map1") == 0 || mapString2.compare("Map1") == 0 ||
						mapString1.compare("Map11") == 0 || mapString2.compare("Map11") == 0)
					{
						Log("INFO  | LolSceneSwitch::MonitorThread | Map is Summoners Rift!");
						instance->currentMap = Map::SUMMONERS_RIFT;
						newMapInfo = true;
					}
					else if (mapString1.compare("Map8") == 0 || mapString2.compare("Map8") == 0)
					{
						Log("INFO  | LolSceneSwitch::MonitorThread | Map is Crystal Scar!");
						instance->currentMap = Map::CRYSTAL_SCAR;
						newMapInfo = true;
					}
					else if (mapString1.compare("Map10") == 0 || mapString2.compare("Map10") == 0)
					{
						Log("INFO  | LolSceneSwitch::MonitorThread | Map is Twisted Treeline!");
						instance->currentMap = Map::TWISTED_TREELINE;
						newMapInfo = true;
					}
					else if (mapString1.compare("Map12") == 0 || mapString2.compare("Map12") == 0)
					{
						Log("INFO  | LolSceneSwitch::MonitorThread | Map is Howling Abyss!");
						instance->currentMap = Map::HOWLING_ABYSS;
						newMapInfo = true;
					}
				}

				if (reader != nullptr && (instance->settings.scenes[State::GAMEOUT].single.IsEmpty() ||
					(gameWindow != nullptr && HasFocus(gameWindow))))
				{
					state = reader->GetState();
				}
				else
				{
					state = State::GAMEOUT;
				}
			}
			else
			{
				Log("INFO  | LolSceneSwitch::MonitorThread | LoL game process exited!");
				ingame = false;
				postGame = true;
				CloseHandle(gameProcess);
				gameProcess = nullptr;
				gameWindow = nullptr;
				reader = nullptr;
				instance->currentMap = Map::UNKNOWN;
				mapAddress = 0;
			}
		}

		if (state != oldState || newMapInfo)
		{
			// Something has changed!!!
			Log("INFO  | LolSceneSwitch::MonitorThread | New state:", static_cast<long long>(state));

			oldState = state;
			newMapInfo = false;
			instance->ChangeScene(state);
		}
		Sleep(INTERVALL);
	}

	return 0;
}



// search strings
std::array<char const *, 3> const LogReader::searchStrings = { "Set focus to app",
															   "Start Main Loop", 
															   "End game message processing!" };

LogReader::LogReader(_In_ HANDLE file) : file(file)
{
}


State LogReader::GetState()
{
	if (index < searchStrings.size())
	{
		char buffer[BUFFER_SIZE];
		char * found = nullptr;
		DWORD bytesRead = 0;
		
		while (ReadFile(file, buffer, BUFFER_SIZE - 1, &bytesRead, nullptr) && bytesRead)
		{
			buffer[bytesRead] = '\0';
			found = strstr(buffer, searchStrings[index]);
			if (found != nullptr)
			{
				SetFilePointer(file, -static_cast<signed long>(bytesRead) + (found - buffer + strlen(searchStrings[index])), nullptr, FILE_CURRENT);
				index += 1;
				if (index >= searchStrings.size())
				{
					// last message of interest has been read already
					break;
				}
			}
			else
			{
				SetFilePointer(file, -static_cast<signed long>(strlen(searchStrings[index])) + 1, nullptr, FILE_CURRENT);
				if (bytesRead < BUFFER_SIZE - 1)
				{
					// nothing more to read here
					break;
				}
			}
		}
	}

	if (index == 0)
	{
		return State::GAMEOUT;
	}
	else if (index == 1)
	{
		return State::LOADSCREEN;
	}
	else if (index == 2)
	{
		return State::GAME;
	}
	else if (index == 3)
	{
		return State::ENDGAME;
	}
	else
	{
		return State::GAMEOUT;
	}
}