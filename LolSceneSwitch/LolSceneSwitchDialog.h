#pragma once

#include "LolSceneSwitchSettings.h"
#include <vector>

class SceneSelector
{
private:

	HWND const chkEnabled;
	HWND const chkUseMultiple;
	HWND * const cmbScenes;
	HWND * const lblScenes;
	unsigned int const length;
	bool const multiple;

public:

	SceneSelector(HWND hWndDialog, HWND chkEnabled, HWND cmbScene);
	SceneSelector(HWND hWndDialog, HWND chkEnabled, HWND chkUseMultiple, int startID, unsigned int length);
	~SceneSelector();

	void EnableControls(bool disableAll) const;
	void SetEnabled() const;
	void SetEnabled(bool useMultiple) const;

	void AddScene(CTSTR scene) const;

	void SetSelectedScenes(String const * scenes) const;
	void GetSelectedScenes(String * scenes) const;
	bool UseMultiple() const;
};

class LolSceneSwitchDialog
{
private:
	
	LolSceneSwitchSettings & settings;

	HWND hWndDialog;
	std::vector<SceneSelector> sceneSelectors;

	void LoadSettings() const;
	void ApplySettings();

	void GetControls(HWND hWnd);
	void EnableControls() const;
	static INT_PTR CALLBACK ConfigDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:

	LolSceneSwitchDialog(LolSceneSwitchSettings & settings);

	void Show(HWND hWnd, HINSTANCE hInstDLL) const;
};