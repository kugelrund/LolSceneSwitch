#pragma once

#include <Windows.h>



void InitLog(_In_ TCHAR const * pluginDataPath);
void CloseLog();
void Log(_In_ char const * text);
void Log(_In_ char const * text, _In_opt_ char const * value);
void Log(_In_ char const * text, _In_opt_ WCHAR const * value);
void Log(_In_ char const * text, long long value);
void Log(_In_ char const * text, long long value, bool hex);
void LogLastError();
