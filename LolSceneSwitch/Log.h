#pragma once

#include <Windows.h>

void InitLog(TCHAR const * pluginDataPath);
void CloseLog();
void Log(char const * text);
void Log(char const * text, WCHAR const * value);
void Log(char const * text, int value);
