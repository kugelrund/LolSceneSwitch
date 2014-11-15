#pragma once

#include "OBSApi.h"
#include <map>



enum class State
{
	CLIENT, CLIENTOUT, CHAMPSELECT, POSTGAME, LOADSCREEN, GAME, ENDGAME, GAMEOUT
};


struct MapScenes
{
	String summonersRift;
	String crystalScar;
	String twistedTreeline;
	String howlingAbyss;

	bool IsEmpty()
	{
		return summonersRift.IsEmpty() && crystalScar.IsEmpty() &&
			twistedTreeline.IsEmpty() && howlingAbyss.IsEmpty();
	}
};


struct StateScenes
{
	String single;
	MapScenes mapScenes;
	bool useMaps;
};


class Settings
{
private:

	ConfigFile config;

public:

	bool enabled;
	String lolPath;
	std::map<State, StateScenes> scenes;
	unsigned int intervall;
	
	Settings();
	~Settings();

	void LoadDefaults();
	void LoadSettings();
	void SaveSettings();
};