#include "LolSceneSwitchSettings.h"
#include "Log.h"

#define CONFIG_FILENAME TEXT("\\LolSceneSwitch.ini")

LolSceneSwitchSettings::LolSceneSwitchSettings()
{
	LoadDefaults();
	config.Open(OBSGetPluginDataPath() + CONFIG_FILENAME, true);
	LoadSettings();
}

LolSceneSwitchSettings::~LolSceneSwitchSettings()
{
	config.Close();
}

void LolSceneSwitchSettings::LoadDefaults()
{
	enabled = true;
	lolPath = String();
	closedScene = String();
	tabbedoutScene = String();
	loadscreenScene[SUMMONERS_RIFT] = String();
	loadscreenScene[CRYSTAL_SCAR] = String();
	loadscreenScene[TWISTED_TREELINE] = String();
	loadscreenScene[HOWLING_ABYSS] = String();
	gameScene[SUMMONERS_RIFT] = String();
	gameScene[CRYSTAL_SCAR] = String();
	gameScene[TWISTED_TREELINE] = String();
	gameScene[HOWLING_ABYSS] = String();
	endgameScene[SUMMONERS_RIFT] = String();
	endgameScene[CRYSTAL_SCAR] = String();
	endgameScene[TWISTED_TREELINE] = String();
	endgameScene[HOWLING_ABYSS] = String();
	loadscreenMaps = false;
	gameMaps = false;
	endgameMaps = false;
	intervall = 300;
}

void LolSceneSwitchSettings::LoadSettings()
{
	enabled = config.GetInt(TEXT("General"), TEXT("enabled"), enabled ? 1 : 0) != 0;
	lolPath = config.GetString(TEXT("General"), TEXT("lolPath"), lolPath);
	closedScene = config.GetString(TEXT("General"), TEXT("closedScene"), closedScene);
	tabbedoutScene = config.GetString(TEXT("General"), TEXT("tabbedoutScene"), tabbedoutScene);
	loadscreenScene[SUMMONERS_RIFT] = config.GetString(TEXT("General"), TEXT("loadscreenScene0"), loadscreenScene[SUMMONERS_RIFT]);
	loadscreenScene[CRYSTAL_SCAR] = config.GetString(TEXT("General"), TEXT("loadscreenScene1"), loadscreenScene[CRYSTAL_SCAR]);
	loadscreenScene[TWISTED_TREELINE] = config.GetString(TEXT("General"), TEXT("loadscreenScene2"), loadscreenScene[TWISTED_TREELINE]);
	loadscreenScene[HOWLING_ABYSS] = config.GetString(TEXT("General"), TEXT("loadscreenScene3"), loadscreenScene[HOWLING_ABYSS]);
	gameScene[SUMMONERS_RIFT] = config.GetString(TEXT("General"), TEXT("gameScene0"), gameScene[SUMMONERS_RIFT]);
	gameScene[CRYSTAL_SCAR] = config.GetString(TEXT("General"), TEXT("gameScene1"), gameScene[CRYSTAL_SCAR]);
	gameScene[TWISTED_TREELINE] = config.GetString(TEXT("General"), TEXT("gameScene2"), gameScene[TWISTED_TREELINE]);
	gameScene[HOWLING_ABYSS] = config.GetString(TEXT("General"), TEXT("gameScene3"), gameScene[HOWLING_ABYSS]);
	endgameScene[SUMMONERS_RIFT] = config.GetString(TEXT("General"), TEXT("endgameScene0"), endgameScene[SUMMONERS_RIFT]);
	endgameScene[CRYSTAL_SCAR] = config.GetString(TEXT("General"), TEXT("endgameScene1"), endgameScene[CRYSTAL_SCAR]);
	endgameScene[TWISTED_TREELINE] = config.GetString(TEXT("General"), TEXT("endgameScene2"), endgameScene[TWISTED_TREELINE]);
	endgameScene[HOWLING_ABYSS] = config.GetString(TEXT("General"), TEXT("endgameScene3"), endgameScene[HOWLING_ABYSS]);
	loadscreenMaps = config.GetInt(TEXT("General"), TEXT("loadscreenMaps"), loadscreenMaps ? 1 : 0) != 0;
	gameMaps = config.GetInt(TEXT("General"), TEXT("gameMaps"), gameMaps ? 1 : 0) != 0;
	endgameMaps = config.GetInt(TEXT("General"), TEXT("endgameMaps"), endgameMaps ? 1 : 0) != 0;
	intervall = config.GetInt(TEXT("General"), TEXT("intervall"), intervall);
}

void LolSceneSwitchSettings::SaveSettings()
{
	config.SetInt(TEXT("General"), TEXT("enabled"), enabled ? 1 : 0);
	config.SetString(TEXT("General"), TEXT("lolPath"), lolPath);
	config.SetString(TEXT("General"), TEXT("closedScene"), closedScene);
	config.SetString(TEXT("General"), TEXT("tabbedoutScene"), tabbedoutScene);
	config.SetString(TEXT("General"), TEXT("loadscreenScene0"), loadscreenScene[SUMMONERS_RIFT]);
	config.SetString(TEXT("General"), TEXT("loadscreenScene1"), loadscreenScene[CRYSTAL_SCAR]);
	config.SetString(TEXT("General"), TEXT("loadscreenScene2"), loadscreenScene[TWISTED_TREELINE]);
	config.SetString(TEXT("General"), TEXT("loadscreenScene3"), loadscreenScene[HOWLING_ABYSS]);
	config.SetString(TEXT("General"), TEXT("gameScene0"), gameScene[SUMMONERS_RIFT]);
	config.SetString(TEXT("General"), TEXT("gameScene1"), gameScene[CRYSTAL_SCAR]);
	config.SetString(TEXT("General"), TEXT("gameScene2"), gameScene[TWISTED_TREELINE]);
	config.SetString(TEXT("General"), TEXT("gameScene3"), gameScene[HOWLING_ABYSS]);
	config.SetString(TEXT("General"), TEXT("endgameScene0"), endgameScene[SUMMONERS_RIFT]);
	config.SetString(TEXT("General"), TEXT("endgameScene1"), endgameScene[CRYSTAL_SCAR]);
	config.SetString(TEXT("General"), TEXT("endgameScene2"), endgameScene[TWISTED_TREELINE]);
	config.SetString(TEXT("General"), TEXT("endgameScene3"), endgameScene[HOWLING_ABYSS]);
	config.SetInt(TEXT("General"), TEXT("loadscreenMaps"), loadscreenMaps ? 1 : 0);
	config.SetInt(TEXT("General"), TEXT("gameMaps"), gameMaps ? 1 : 0);
	config.SetInt(TEXT("General"), TEXT("endgameMaps"), endgameMaps ? 1 : 0);
	config.SetInt(TEXT("General"), TEXT("intervall"), intervall);
}