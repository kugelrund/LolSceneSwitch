#pragma once

#include "Settings.h"
#include <vector>



class SceneSelector
{
private:

	HWND const chkEnabled;
	HWND const cmbScene;
	HWND const chkMaps;
	HWND const btnMaps;
	bool const mapsAvail;
	StateScenes scenes;

public:

	SceneSelector();
	SceneSelector(_In_ HWND chkEnabled, _In_ HWND cmbScene);
	SceneSelector(_In_ HWND chkEnabled, _In_ HWND cmbScene, _In_ HWND chkMaps, _In_ HWND btnMaps);

	void EnableControls(bool disableAll) const;
	void SetEnabled() const;
	void SetEnabled(bool useMultiple) const;

	void SetMapScenes(MapScenes scenes);
	MapScenes GetMapScenes();
	void AddScenes(std::vector<CTSTR> scenes) const;
	void LoadSettings(StateScenes scenes);
	void GetSettings(_Inout_ StateScenes &scenes);

	bool UseMaps();
};


class SettingsDialog
{
private:
	
	Settings & settings;
	HINSTANCE hInstDLL;

	HWND hWndDialog;
	std::vector<CTSTR> scenes;
	std::map<State, SceneSelector> selectors;
	std::map<unsigned int, State> selectorIndices;

	void LoadSettings();
	void ApplySettings();

	void GetControls(_In_ HWND hWnd);
	void EnableControls() const;
	static INT_PTR CALLBACK ConfigDlgProc(_In_ HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:

	SettingsDialog(_In_ Settings &settings);
	void Show(_In_ HWND hWnd, _In_ HINSTANCE hInstDLL);
};