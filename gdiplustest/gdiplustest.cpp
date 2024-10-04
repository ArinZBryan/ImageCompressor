// gdiplustest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define GDIPVER 0x0110
#include <windows.h>
#include <gdiplus.h>
#include <gdiplusheaders.h>
#include <Gdipluspixelformats.h>
#include "libCLI.h"
#include <iostream>
#include <set>
#pragma comment (lib,"Gdiplus.lib")

namespace gdip = Gdiplus;

const wchar_t sourceFile[] = L"C:\\Users\\arinb\\source\\repos\\ImageCompressor\\Tests\\small\\small_colour_565.bmp";
const wchar_t destFile[] = L"C:\\Users\\arinb\\source\\repos\\ImageCompressor\\Tests\\output\\small_colour_565_to_4bitIndexed.bmp";

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

// Custom deleter to properly free the allocated memory
struct ColorPaletteDeleter {
	void operator()(Gdiplus::ColorPalette* palette) const {
		delete[] palette; // Use free because we are using malloc for allocation
	}
};
std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter> allocatePalette(int colors, uint32_t flags) {
	size_t paletteSize = sizeof(gdip::ColorPalette) + (colors - 1) * sizeof(gdip::ARGB);
	//gdip::ColorPalette* palette = (gdip::ColorPalette*)malloc(paletteSize);
	gdip::ColorPalette* palette = reinterpret_cast<gdip::ColorPalette*>(new uint8_t[paletteSize]);
	palette->Count = colors;
	palette->Flags = flags;
	return std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter>(palette);
}
std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter> allocatePalette(size_t paletteSize, uint32_t flags) {
	//gdip::ColorPalette* palette = (gdip::ColorPalette*)malloc(paletteSize);
	gdip::ColorPalette* palette = reinterpret_cast<gdip::ColorPalette*>(new uint8_t[paletteSize]);
	palette->Count = (paletteSize - (sizeof(gdip::ColorPalette) - sizeof(gdip::ARGB))) / sizeof(gdip::ARGB);
	palette->Flags = flags;
	return std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter>(palette);
}
std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter> makeSmallOptimalPalette(size_t maxSize, gdip::Bitmap& image, bool imageIsGreyscale) {

	if (maxSize > 256) { maxSize = 256; } //Image Palettes larger than 256 are not supported.

	std::set<gdip::ARGB> uniqueColors;

	int width = image.GetWidth();
	int height = image.GetHeight();

	gdip::BitmapData bitmapData;
	gdip::Rect rect(0, 0, width, height);
	image.LockBits(&rect, gdip::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

	// Iterate through the pixels to find unique colors
	for (int y = 0; y < height; ++y)
	{
		uint8_t* row = (uint8_t*)bitmapData.Scan0 + y * bitmapData.Stride;
		for (uint8_t x = 0; x < width; ++x)
		{
			gdip::ARGB color = *(gdip::ARGB*)(row + x * 4);
			uniqueColors.insert(color);
		}
	}

	// Unlock the bitmap
	image.UnlockBits(&bitmapData);

	if (uniqueColors.size() < 1 || maxSize < 1) {
		return nullptr;
	}
	auto palette = allocatePalette(static_cast<int>(min(maxSize, uniqueColors.size())), imageIsGreyscale ? gdip::PaletteFlagsGrayScale : 0);
	gdip::Bitmap::InitializePalette(palette.get(), gdip::PaletteTypeOptimal, min(maxSize, uniqueColors.size()), false, &image);

	return palette;
}


int main()
{
	std::cout << "Using File: ";
	std::wcout << sourceFile << std::endl;

	//Initialise Windows GDI+
	ULONG_PTR gdiplusToken;
	gdip::GdiplusStartupInput gdiplusStartupInput;
	gdip::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	//Load Bitmap from file
	gdip::Bitmap* bitmap = new Gdiplus::Bitmap(sourceFile);
	if (bitmap->GetLastStatus() != Gdiplus::Ok)
	{
		std::cerr << "[Error] Failed to load bitmap." << std::endl;
		return 1;
	}

	// NOTE - the bitmap->ConvertFormat(...) function REPLACES the image data in the bitmap. Make sure to make a copy if the original is still needed

	//Convert Image to 24bppRGB colour
	//bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);

	//Convert Image to 16bpp555 colour - NOTE: MetaOutput Preview says colour format is 32Bit RGB, but VS properties says 16bpp 555
	//bitmap->ConvertFormat(PixelFormat16bppRGB555, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);

	//Convert Image to 16bpp565 colour - NOTE: MetaOutput Preview says colour format is 32Bit RGB, but VS properties says 16bpp 565
	//bitmap->ConvertFormat(PixelFormat16bppRGB565, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);

	//Convert Image to 16bpp greyscale does not work

	//Convert Image to 8bit indexed with optimal colour palette with no dithering - NOTE: gdip::DitherTypeNone breaks this (for some reason) - use any other dithertype
	//auto palette = makeSmallOptimalPalette(256, *bitmap, false);
	//bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, palette.get(), 0);

	//Convert Image to 8bit indexed with optimal colour palette with dithering - NOTE: gdip::DitherTypeNone breaks this (for some reason) - use any other dithertype
	//auto palette = makeSmallOptimalPalette(256, *bitmap, false);
	//bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeErrorDiffusion, gdip::PaletteTypeCustom, palette.get(), 0);

	//Convert Image to 4bit indexed with optimal colour palette with no dithering - NOTE: gdip::DitherTypeNone breaks this (for some reason) - use any other dithertype
	//auto palette = makeSmallOptimalPalette(16, *bitmap, false);
	//bitmap->ConvertFormat(PixelFormat4bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, palette.get(), 0);


	CLSID bmpCLSID;
	GetEncoderClsid(L"image/bmp", &bmpCLSID);
	if (bitmap->Save(destFile, &bmpCLSID) != gdip::Ok) {
		std::cerr << "[Error] Failed to save bitmap." << std::endl;
		return 1;
	}

	return 0;
}