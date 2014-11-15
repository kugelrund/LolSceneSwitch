#pragma once

#include "Settings.h"

extern "C" __declspec(dllexport) bool LoadPlugin();
extern "C" __declspec(dllexport) void UnloadPlugin();
extern "C" __declspec(dllexport) CTSTR GetPluginName();
extern "C" __declspec(dllexport) CTSTR GetPluginDescription();
extern "C" __declspec(dllexport) void OnStartStream();
extern "C" __declspec(dllexport) void OnStopStream();
extern "C" __declspec(dllexport) void ConfigPlugin(_In_ HWND);


enum class Map
{
	UNKNOWN, SUMMONERS_RIFT, CRYSTAL_SCAR, TWISTED_TREELINE, HOWLING_ABYSS
};


class LolSceneSwitch
{
private:

	Settings settings;

	HANDLE lolProcessMonitor; 
	bool runMonitoring;

	Map currentMap;
		
	void ChangeScene(State state);
	void SetScene(String scene) const;

	static DWORD WINAPI MonitorThread(_In_ LPVOID lpParam);
	
public:

	LolSceneSwitch();
	~LolSceneSwitch();

	bool IsMonitoring() const;
	Settings & GetSettingsRef();
	
	void StartMonitoring();
	void EndMonitoring();
};


class LogReader
{
private:

	HANDLE file;
	unsigned int index = 0;

public:

	LogReader(_In_ HANDLE file);
	State GetState();
};