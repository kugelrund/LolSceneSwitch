#include "Log.h"
#include <fstream>
#include <string>
#include <Pathcch.h>

using namespace std;
ofstream ofs;

void InitLog(TCHAR const * pluginDataPath)
{
	TCHAR const LOGNAME[] = TEXT("LolSceneSwitch.log");
	TCHAR logPath[MAX_PATH];

	if (SUCCEEDED(PathCchCombine(logPath, MAX_PATH, pluginDataPath, LOGNAME)))
	{
		ofs.open(logPath, ofstream::out | ofstream::trunc);
	}
}

void CloseLog()
{
	ofs.close();
}

void Log(char const * text)
{
	ofs << text << endl;
}

void Log(char const * text, WCHAR const * value)
{
	size_t numOut;
	char conv[256];
	wcstombs_s(&numOut, conv, value, 256);
	ofs << text << "'" << conv << "'" << endl;
}

void Log(char const * text, int value)
{
	ofs << text << "'" << value << "'" << std::endl;
}