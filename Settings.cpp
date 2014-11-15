#include "Settings.h"
#include "Log.h"

#define CONFIG_FILENAME TEXT("\\LolSceneSwitch.ini")
#ifdef UNICODE
typedef std::wstring tstring;
#define to_tstring std::to_wstring
#else
typedef std::string tstring;
#define to_tstring std::to_string
#endif



Settings::Settings()
{
	scenes.emplace(State::CLIENT, StateScenes());
	scenes.emplace(State::CLIENTOUT, StateScenes());
	scenes.emplace(State::CHAMPSELECT, StateScenes());
	scenes.emplace(State::POSTGAME, StateScenes());
	scenes.emplace(State::LOADSCREEN, StateScenes());
	scenes.emplace(State::GAME, StateScenes());
	scenes.emplace(State::ENDGAME, StateScenes());
	scenes.emplace(State::GAMEOUT, StateScenes());

	LoadDefaults();
	config.Open(OBSGetPluginDataPath() + CONFIG_FILENAME, true);
	LoadSettings();
}


Settings::~Settings()
{
	config.Close();
}


void Settings::LoadDefaults()
{
	enabled = true;
	lolPath = TEXT("");
	for (auto &scene : scenes)
	{
		scene.second.single = TEXT("");
		scene.second.mapScenes.summonersRift = TEXT("");
		scene.second.mapScenes.crystalScar = TEXT("");
		scene.second.mapScenes.twistedTreeline = TEXT("");
		scene.second.mapScenes.howlingAbyss = TEXT("");
		scene.second.useMaps = false;
	}
	intervall = 300;
}


void Settings::LoadSettings()
{
	enabled = config.GetInt(TEXT("General"), TEXT("enabled"), enabled ? 1 : 0) != 0;
	lolPath = config.GetString(TEXT("General"), TEXT("lolPath"), lolPath);
	for (auto &scene : scenes)
	{
		tstring id = to_tstring(static_cast<int>(scene.first));
		scene.second.single = config.GetString(TEXT("General"), 
											   (id + TEXT(".single")).c_str(), 
											   scene.second.single);
		scene.second.useMaps = config.GetInt(TEXT("General"), 
											 (id + TEXT(".useMaps")).c_str(), 
											 scene.second.useMaps ? 1 : 0) != 0;
		scene.second.mapScenes.summonersRift = config.GetString(TEXT("General"), 
																(id + TEXT(".maps.summonersRift")).c_str(), 
																scene.second.mapScenes.summonersRift);
		scene.second.mapScenes.crystalScar = config.GetString(TEXT("General"),
																(id + TEXT(".maps.crystalScar")).c_str(),
																scene.second.mapScenes.crystalScar);
		scene.second.mapScenes.twistedTreeline = config.GetString(TEXT("General"),
																(id + TEXT(".maps.twistedTreeline")).c_str(),
																scene.second.mapScenes.twistedTreeline);
		scene.second.mapScenes.howlingAbyss = config.GetString(TEXT("General"),
																(id + TEXT(".maps.howlingAbyss")).c_str(),
																scene.second.mapScenes.howlingAbyss);
	}
	intervall = config.GetInt(TEXT("General"), TEXT("intervall"), intervall);
}


void Settings::SaveSettings()
{
	config.SetInt(TEXT("General"), TEXT("enabled"), enabled ? 1 : 0);
	config.SetString(TEXT("General"), TEXT("lolPath"), lolPath);
	for (auto scene : scenes)
	{
		tstring id = to_tstring(static_cast<int>(scene.first));
		config.SetString(TEXT("General"), (id + TEXT(".single")).c_str(), scene.second.single);
		config.SetInt(TEXT("General"), (id + TEXT(".useMaps")).c_str(), scene.second.useMaps ? 1 : 0);
		config.SetString(TEXT("General"), (id + TEXT(".maps.summonersRift")).c_str(), 
						 scene.second.mapScenes.summonersRift);
		config.SetString(TEXT("General"), (id + TEXT(".maps.crystalScar")).c_str(), 
						 scene.second.mapScenes.crystalScar);
		config.SetString(TEXT("General"), (id + TEXT(".maps.twistedTreeline")).c_str(), 
						 scene.second.mapScenes.twistedTreeline);
		config.SetString(TEXT("General"), (id + TEXT(".maps.howlingAbyss")).c_str(),
						 scene.second.mapScenes.howlingAbyss);
	}
	config.SetInt(TEXT("General"), TEXT("intervall"), intervall);
}