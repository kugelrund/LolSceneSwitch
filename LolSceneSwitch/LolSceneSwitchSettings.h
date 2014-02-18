#pragma once

#include "OBSApi.h"

enum LoLMaps
{
	SINGLE = 0, SUMMONERS_RIFT = 0, CRYSTAL_SCAR = 1, TWISTED_TREELINE = 2, HOWLING_ABYSS = 3
};

struct LolSceneSwitchSettings
{
private:

	ConfigFile config;

public:

	bool enabled;
	String lolPath;
	String closedScene;
	String tabbedoutScene;
	String loadscreenScene[4];
	String gameScene[4];
	String endgameScene[4];
	bool loadscreenMaps;
	bool gameMaps;
	bool endgameMaps;
	unsigned int intervall;

	LolSceneSwitchSettings();
	~LolSceneSwitchSettings();

	void LoadDefaults();
	void LoadSettings();
	void SaveSettings();
};