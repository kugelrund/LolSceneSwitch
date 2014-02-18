#pragma once

#include "LolSceneSwitchSettings.h"

extern "C" __declspec(dllexport) bool LoadPlugin();
extern "C" __declspec(dllexport) void UnloadPlugin();
extern "C" __declspec(dllexport) CTSTR GetPluginName();
extern "C" __declspec(dllexport) CTSTR GetPluginDescription();
extern "C" __declspec(dllexport) void OnStartStream();
extern "C" __declspec(dllexport) void OnStopStream();
extern "C" __declspec(dllexport) void ConfigPlugin(HWND);

class LolSceneSwitch
{
private:

	LolSceneSwitchSettings settings;

	HANDLE lolProcessMonitor; 
	HANDLE const eventEndMonitoring;
	bool runMonitoring;
	bool lolProcessClosed;

	FILETIME lolStartTime;
	DWORD lolPid;

	String currentGameScene;
	bool stateChanged;
		

	void SetCurrentScene(String const & scene);
	void ChangeScene() const;
	void RevertScene() const;

	static DWORD WINAPI ProcessMonitorThread(LPVOID lpParam);
	template <unsigned int N> bool WaitForStringInLog(char const (&searchString)[N], HANDLE file) const;
	static DWORD WINAPI LogMonitorThread(LPVOID lpParam);
	static DWORD WINAPI WindowFocusMonitorThread(LPVOID lpParam);
	
public:

	LolSceneSwitch();
	~LolSceneSwitch();

	bool IsMonitoring() const;
	LolSceneSwitchSettings & GetSettingsRef();
	
	void StartMonitoring();
	void EndMonitoring();
};