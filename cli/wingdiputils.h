#pragma once
#include <string>
#include <iostream>
#ifndef _WINDOWS_
#include <windows.h>
#endif
#ifndef _GDIPLUS_H
#include <gdiplus.h>
#endif

std::wstring to_wide(const std::string& multi);
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
bool GdiplusErrorHandler(Gdiplus::Status& res);