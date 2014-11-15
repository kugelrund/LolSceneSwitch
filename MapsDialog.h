#pragma once

#include "Settings.h"
#include <vector>



class MapsDialog
{
private:

	HWND cmbSummonersRift;
	HWND cmbCrystalScar;
	HWND cmbTwistedTreeline;
	HWND cmbHowlingAbyss;
	MapScenes selectedScenes;

	std::vector<CTSTR> scenes;
	static INT_PTR CALLBACK DialogProc(_In_ HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void GetControls(_In_ HWND hWnd);
	void LoadScenes();
	void ApplyScenes();

public:

	MapsDialog(std::vector<CTSTR> scenes, MapScenes mapScenes);
	_Success_(return) bool Show(_In_ HINSTANCE hInstDLL, _In_ HWND hWnd, _Out_ MapScenes * scenes);
};