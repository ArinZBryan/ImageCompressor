#pragma once
#include <string>
#include <iostream>
#include <windows.h>
#include <gdiplus.h>
#include <gdiplusheaders.h>

std::wstring to_wide(const std::string& multi);
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
bool GdiplusErrorHandler(Gdiplus::Status& res);