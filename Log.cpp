#include "Log.h"
#include <fstream>
#include <sstream>

#ifdef UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif



std::ofstream ofs;


void InitLog(_In_ TCHAR const * pluginDataPath)
{
	tstring logPath(pluginDataPath);
	logPath.append(TEXT("\\LolSceneSwitch.log"));

	ofs.open(logPath, std::ofstream::out | std::ofstream::trunc);
}


void CloseLog()
{
	ofs.close();
}


void Log(_In_ char const * text)
{
	if (!ofs.bad())
	{
		ofs << text << std::endl;
	}
}


void Log(_In_ char const * text, _In_opt_ char const * value)
{
	if (!ofs.bad())
	{
		if (value != nullptr)
		{
			ofs << text << " '" << value << "'" << std::endl;
		}
		else
		{
			ofs << text << " 'NULL'" << std::endl;
		}
	}
}


void Log(_In_ char const * text, _In_opt_ WCHAR const * value)
{
	if (!ofs.bad())
	{
		if (value != nullptr)
		{
			size_t length;
			char converted[256];
			wcstombs_s(&length, converted, value, 256);
			ofs << text << " '" << converted << "'" << std::endl;
		}
		else
		{
			ofs << text << " 'NULL'" << std::endl;
		}
	}
}


void Log(_In_ char const * text, long long value)
{
	Log(text, value, false);
}


void Log(_In_ char const * text, long long value, bool hex)
{
	if (hex)
	{
		if (!ofs.bad())
		{
			std::stringstream stream;
			stream << std::hex << value;
			ofs << text << " '" << stream.str() << "'" << std::endl;
		}
	}
	else
	{
		ofs << text << " '" << value << "'" << std::endl;
	}
}


void LogLastError()
{
	if (!ofs.bad())
	{
		DWORD eNum = GetLastError();
		TCHAR sysMsg[256];
		if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, eNum,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), sysMsg, 256, NULL) == 0)
		{
			Log("ERROR | LogLastError | Couldn't format error message with number", eNum, true);
			return;
		}

		// find end of string
		TCHAR *p = sysMsg;
		while ((*p > 31) || (*p == 9))
		{
			++p;
		}

		// remove unused end
		do
		{
			*p-- = 0;
		} while ((p >= sysMsg) && ((*p == '.') || (*p < 33)));

		Log("      | GetLastError |", sysMsg);
	}
}