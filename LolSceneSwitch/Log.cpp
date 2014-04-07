#include "Log.h"
#include <fstream>
#include <strsafe.h>

using namespace std;
ofstream ofs;

void InitLog(TCHAR const * pluginDataPath)
{
	TCHAR const LOGNAME[] = TEXT("\\LolSceneSwitch.log");
	size_t const nameLength = (sizeof LOGNAME) / (sizeof *LOGNAME);
	size_t pathLength;

	TCHAR logPath[MAX_PATH];

	if (SUCCEEDED(StringCchLength(pluginDataPath, MAX_PATH, &pathLength)) && ((pathLength + nameLength) <= MAX_PATH) &&
		SUCCEEDED(StringCchCopy(logPath, MAX_PATH, pluginDataPath)) && SUCCEEDED(StringCchCat(logPath, MAX_PATH, LOGNAME)))
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
	if (!ofs.bad())
	{
		ofs << text << endl;
	}
}

void Log(char const * text, WCHAR const * value)
{
	if (!ofs.bad())
	{
		if (value != nullptr)
		{
			size_t length;
			char conv[256];
			wcstombs_s(&length, conv, value, 256);
			ofs << text << "'" << conv << "'" << endl;
		}
		else
		{
			ofs << text << "'NULL'" << endl;
		}
	}
}

void Log(char const * text, int value)
{
	if (!ofs.bad())
	{
		ofs << text << "'" << value << "'" << endl;
	}
}