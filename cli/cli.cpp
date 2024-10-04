#define GDIPVER 0x0110
#include <iostream>
#include <set>
#include <windows.h>
#include <gdiplus.h>
#include <gdiplusheaders.h>
#include <gdipluspixelformats.h>
#pragma comment (lib,"Gdiplus.lib")
#include "wingdiputils.h"
#include "libCLI.h"
#include "bitstream.h"
#include "colorconverter.h"
#include "cli.h"

namespace gdip = Gdiplus;

struct CLIArg cliArgCfg[] = {
	CLIArg{ "-w", "--width", "Width of the output image (px)", std::optional<int>(std::nullopt), false },
	CLIArg{ "-h", "--height", "Height of the output image (px)", std::optional<int>(std::nullopt), false },
	CLIArg{ "-c", "--colour-format", "Format of outputted colours - Options: pi1, pi2, pi4, i8r1, i8r2, pg1, pg2, pc3, pg3, pg4, pc6, c555r1 c555r2, c565r1, c565r2, c24r1, c24r2", std::optional<std::string>(std::nullopt), true },
	CLIArg{ "-p", "--palette-format", "Format of palette colours - Options: g2, c3, g3, g4, c6, c555, c565, c24", std::optional<std::string>(std::nullopt), false },
	CLIArg{ "-s", "--source", "File path of input image", std::optional<std::string>(std::nullopt), true },
	CLIArg{ "-d", "--destination", "File path of output image", std::optional<std::string>(std::nullopt), true },
};
const char* defaultArgv[] = {
	"-s",
	"C:\\Users\\arinb\\source\\repos\\ImageCompressor\\Tests\\small\\small_greyscale_8bI.bmp",
	"-c",
	"c24r1"
};

template<typename T, typename... Types>
bool getFromVariantOptional(const std::variant<Types...>& source, T* dest) {
	if (std::holds_alternative<std::optional<T>>(source)) {
		auto optional = std::get<std::optional<T>>(source);
		if (optional.has_value()) {
			*dest = optional.value();
			return true;
		}
	}
	dest = nullptr;
	return false;
}

// Custom deleter to properly free the allocated memory
struct ColorPaletteDeleter {
	void operator()(Gdiplus::ColorPalette* palette) const {
		delete[] palette; 
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
std::unique_ptr<uint8_t[]> makeOutputPalette(std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter>& inputPalette, CompressedImagePaletteFormat paletteFormat) {
	int colSize = 24;
	bitstream palette = bitstream();
	switch (paletteFormat) {
	case CompressedImagePaletteFormat::noPalette: {
		std::cerr << "[Error] Tried to make palette with format of 'No Palette'" << std::endl;
		return std::unique_ptr<uint8_t[]>(new uint8_t[0]);
		break;
	}
	case CompressedImagePaletteFormat::greyscale2Bit: {
		colSize *= 2;
		size_t byteSize = ceilf(inputPalette->Count * colSize / 8.0);
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.pushBits(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toGreyscale2Bit(), colSize);
		}
		std::unique_ptr<uint8_t[]> paletteBuffer(new uint8_t[byteSize]);
		palette.getBytes(paletteBuffer.get(), byteSize);
		return paletteBuffer;
		break;
	}
	case CompressedImagePaletteFormat::greyscale3Bit: {
		colSize *= 3;
		size_t byteSize = ceilf(inputPalette->Count * colSize / 8.0);
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.pushBits(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toGreyscale3Bit(), colSize);
		}
		std::unique_ptr<uint8_t[]> paletteBuffer(new uint8_t[byteSize]);
		palette.getBytes(paletteBuffer.get(), byteSize);
		return paletteBuffer;
		break;
	}
	case CompressedImagePaletteFormat::greyscale4Bit: {
		colSize *= 4;
		size_t byteSize = ceilf(inputPalette->Count * colSize / 8.0);
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.pushBits(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toGreyscale4Bit(), colSize);
		}
		std::unique_ptr<uint8_t[]> paletteBuffer(new uint8_t[byteSize]);
		palette.getBytes(paletteBuffer.get(), byteSize);
		return paletteBuffer;
		break;
	}
	case CompressedImagePaletteFormat::colour3Bit: {
		colSize *= 3;
		size_t byteSize = ceilf(inputPalette->Count * colSize / 8.0);
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.pushBits(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour3Bit(), colSize);
		}
		std::unique_ptr<uint8_t[]> paletteBuffer(new uint8_t[byteSize]);
		palette.getBytes(paletteBuffer.get(), byteSize);
		return paletteBuffer;
		break;
	}
	case CompressedImagePaletteFormat::colour6Bit: {
		colSize *= 6;
		size_t byteSize = ceilf(inputPalette->Count * colSize / 8.0);
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.pushBits(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour6Bit(), colSize);
		}
		std::unique_ptr<uint8_t[]> paletteBuffer(new uint8_t[byteSize]);
		palette.getBytes(paletteBuffer.get(), byteSize);
		return paletteBuffer;
		break;
	}
	case CompressedImagePaletteFormat::colour555: {
		colSize *= 16;
		size_t byteSize = ceilf(inputPalette->Count * colSize / 8.0);
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.pushBits(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour555(), colSize);
		}
		std::unique_ptr<uint8_t[]> paletteBuffer(new uint8_t[byteSize]);
		palette.getBytes(paletteBuffer.get(), byteSize);
		return paletteBuffer;
		break;
	}
	case CompressedImagePaletteFormat::colour565: {
		colSize *= 16;
		size_t byteSize = ceilf(inputPalette->Count * colSize / 8.0);
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.pushBits(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour565(), colSize);
		}
		std::unique_ptr<uint8_t[]> paletteBuffer(new uint8_t[byteSize]);
		palette.getBytes(paletteBuffer.get(), byteSize);
		return paletteBuffer;
		break;
	}
	case CompressedImagePaletteFormat::colourFull: {
		colSize *= 24;
		size_t byteSize = ceilf(inputPalette->Count * colSize / 8.0);
		for (int i = 0; i < inputPalette->Count; i++) {
			ConvertibleColour::colour24_t col = ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour24Bit();
			palette.pushBits(col.R, 8);
			palette.pushBits(col.G, 8);
			palette.pushBits(col.B, 8);
		}
		std::unique_ptr<uint8_t[]> paletteBuffer(new uint8_t[byteSize]);
		palette.getBytes(paletteBuffer.get(), byteSize);
		return paletteBuffer;
		break;
	}
	}
}

std::vector<uint8_t> runLengthEncode(bitstream bits, int unitLength, int packLength) {
	size_t byteBufferLength = ceilf(unitLength / 8.0);
	uint8_t* byteBuffer = static_cast<uint8_t*>(malloc(byteBufferLength));
	int runLength = 0;
	bool firstTime = true;
	while (bits.bitLength() > 0) {
		if (firstTime) {
			bits.pop
		}
	}
}

int main(int argc, const char** argv)
{
	std::unordered_map<std::string, CLIArg> cliArgs;
	if (argc <= 1) {
		cliArgs = parseArgs(sizeof(defaultArgv) / sizeof(defaultArgv[0]), defaultArgv, sizeof(cliArgCfg) / sizeof(cliArgCfg[0]), cliArgCfg);
		std::cout << "[Warn] No Args Supplied, using default args" << std::endl;
	}
	else {
		//Parse and check args
		cliArgs = parseArgs(argc, argv, sizeof(cliArgCfg) / sizeof(cliArgCfg[0]), cliArgCfg);
		std::cout << "[Info] Supplied Arguments: " << std::endl;
	}
	for (std::pair<std::string, CLIArg> arg : cliArgs) { std::cout << cliargtoa(arg.second); }
	std::cout << std::endl;
	if (cliArgs.size() == 0) {
		std::cerr << "[Error] No Arguments. Terminating." << std::endl; return 1;
	}

	//Initialise Windows GDI+
	ULONG_PTR gdiplusToken;
	gdip::GdiplusStartupInput gdiplusStartupInput;
	gdip::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

	// Load the bitmap from a file
	CLIArg fileSource = cliArgs.at("--source");
	std::string narrowFileSourcePath;
	if (!getFromVariantOptional(fileSource.value, &narrowFileSourcePath)) {
		std::cerr << "[Error] Failed to load bitmap." << std::endl;
		return 1;
	}

	std::wstring wideFileSourcePath = to_wide(narrowFileSourcePath);
	gdip::Bitmap* bitmap = new Gdiplus::Bitmap(wideFileSourcePath.c_str());
	if (bitmap->GetLastStatus() != Gdiplus::Ok)
	{
		std::cerr << "[Error] Failed to load bitmap." << std::endl;
		return 1;
	}

	//Flip image if necessary
	gdip::BitmapData bitmapData;
	gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
	bitmap->LockBits(&rect, gdip::ImageLockModeRead | gdip::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);
	//calculate size of bitmapData
	size_t contentSize = bitmapData.Height * abs(bitmapData.Stride);
	uint8_t* contentBuf = static_cast<uint8_t*>(malloc(contentSize));
	uint8_t* startPoint = bitmapData.Stride < 0 ? contentBuf + contentSize : contentBuf;
	for (int i = 0; i < bitmapData.Height; i++) {
		uint8_t* srcRow = static_cast<uint8_t*>(bitmapData.Scan0) + i * bitmapData.Stride;
		uint8_t* dstRow = startPoint + (sign(bitmapData.Stride) * i);
		memcpy(dstRow, srcRow, bitmapData.Stride);
	}
	memcpy(bitmapData.Scan0, contentBuf, contentSize);
	free(contentBuf);
	bitmapData.Stride = abs(bitmapData.Stride);
	bitmap->UnlockBits(&bitmapData);

	//Resize image if necessary
	int widthDesired = bitmap->GetWidth(), heightDesired = bitmap->GetHeight(), resize = 0;
	if (cliArgs.contains("--width")) {
		if (!getFromVariantOptional(cliArgs.at("--width").value, &widthDesired)) {
			std::cerr << "[Error] Misformatted Argument: --width (-w)" << std::endl << "	Expected: Positive Integer" << std::endl;
			return 1;
		}
		resize |= 0b01;
	}
	if (cliArgs.contains("--height")) {
		if (!getFromVariantOptional(cliArgs.at("--height").value, &heightDesired)) {
			std::cerr << "[Error] Misformatted Argument: --height (-h)" << std::endl << "	Expected: Positive Integer" << std::endl;
			return 1;
		}
		resize |= 0b10;
	}
	if (resize > 0) {
		gdip::Bitmap* workBitmap = new gdip::Bitmap(widthDesired, heightDesired);
		gdip::Graphics g(workBitmap);
		float horizontalScalingFactor = (float)widthDesired / (float)bitmap->GetWidth();
		float verticalScalingFactor = (float)heightDesired / (float)bitmap->GetHeight();
		g.ScaleTransform(horizontalScalingFactor, verticalScalingFactor);
		g.DrawImage(bitmap, 0, 0);
		delete bitmap;
		bitmap = workBitmap;
		std::cout << "[Info] New Dimensions: W:" << bitmap->GetHeight() << " H:" << bitmap->GetWidth() << std::endl;
	}

	CLIArg fileSource = cliArgs.at("--colour-format");
	std::string colourFormatString;
	if (getFromVariantOptional(fileSource.value, &colourFormatString)) {
		std::cerr << "[Error] Invalid Colour Format" << std::endl;
		return 1;
	}
	CompressedImageColourFormat colourFormatDesired;
	if (colourFormatString == "pi1") { CompressedImageColourFormat::packedIndexBit; }
	else if (colourFormatString == "pi2") { CompressedImageColourFormat::packedIndex2Bit; }
	else if (colourFormatString == "pi4") { CompressedImageColourFormat::packedIndex4Bit; }
	else if (colourFormatString == "i8r1") { CompressedImageColourFormat::index8Bit; }
	else if (colourFormatString == "i8r2") { CompressedImageColourFormat::index8Bit; }
	else if (colourFormatString == "pg1") { CompressedImageColourFormat::packedGreyscale1Bit; }
	else if (colourFormatString == "pg2") { CompressedImageColourFormat::packedGreyscale2Bit; }
	else if (colourFormatString == "pc3") { CompressedImageColourFormat::packedColour3Bit; }
	else if (colourFormatString == "pg3") { CompressedImageColourFormat::packedGreyscale3Bit; }
	else if (colourFormatString == "pg4") { CompressedImageColourFormat::packedGreyscale4Bit; }
	else if (colourFormatString == "pc6") { CompressedImageColourFormat::packedColour6Bit; }
	else if (colourFormatString == "c555r1") { CompressedImageColourFormat::colour555; }
	else if (colourFormatString == "c555r2") { CompressedImageColourFormat::colour555; }
	else if (colourFormatString == "c565r1") { CompressedImageColourFormat::colour565; }
	else if (colourFormatString == "c565r2") { CompressedImageColourFormat::colour565; }
	else if (colourFormatString == "c24r1") { CompressedImageColourFormat::colourFull; }
	else if (colourFormatString == "c24r2") { CompressedImageColourFormat::colourFull; }
	else {
		colourFormatDesired = CompressedImageColourFormat::colour565;
		std::cout << "[Info] No colour format supplied, using 16-bit 565 colour, with a run-length of 1" << std::endl;
	}

	bitstream rawDataStream;
	std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter> extractedPalette;
	std::unique_ptr<uint8_t[]> outputPalette(new uint8_t[0]);

	std::string paletteFormatString;
	if (getFromVariantOptional(fileSource.value, &paletteFormatString)) {
		std::cerr << "[Error] Invalid Colour Format" << std::endl;
		return 1;
	}
	CompressedImagePaletteFormat paletteFormatDesired;
	if (paletteFormatString == "g2") { paletteFormatDesired = CompressedImagePaletteFormat::greyscale2Bit; }
	else if (paletteFormatString == "g3") { paletteFormatDesired = CompressedImagePaletteFormat::greyscale3Bit; }
	else if (paletteFormatString == "g4") { paletteFormatDesired = CompressedImagePaletteFormat::greyscale4Bit; }
	else if (paletteFormatString == "c3") { paletteFormatDesired = CompressedImagePaletteFormat::colour3Bit; }
	else if (paletteFormatString == "c6") { paletteFormatDesired = CompressedImagePaletteFormat::colour6Bit; }
	else if (paletteFormatString == "c555") { paletteFormatDesired = CompressedImagePaletteFormat::colour555; }
	else if (paletteFormatString == "c565") { paletteFormatDesired = CompressedImagePaletteFormat::colour565; }
	else if (paletteFormatString == "c24") { paletteFormatDesired = CompressedImagePaletteFormat::colourFull; }
	else {
		paletteFormatDesired = CompressedImagePaletteFormat::noPalette;
	}
	
	std::unique_ptr<uint8_t> outputPalette(new uint8_t[0]);
	switch (colourFormatDesired) {
	case CompressedImageColourFormat::colour555: {
		bitmap->ConvertFormat(PixelFormat16bppRGB555, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		size_t rawSize = bitmapData.PixelFormat == PixelFormat24bppRGB ? 3 : 2;
		for (int row = 0; row < bitmapData.Height; row++) {
			rawDataStream.pushBytes(static_cast<uint8_t*>(bitmapData.Scan0) + row * bitmapData.Stride, rawSize * bitmapData.Width);
		}
		break;
	}
	case CompressedImageColourFormat::colour565: {
		bitmap->ConvertFormat(PixelFormat16bppRGB565, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		size_t rawSize = bitmapData.PixelFormat == PixelFormat24bppRGB ? 3 : 2;
		for (int row = 0; row < bitmapData.Height; row++) {
			rawDataStream.pushBytes(static_cast<uint8_t*>(bitmapData.Scan0) + row * bitmapData.Stride, rawSize * bitmapData.Width);
		}
		break;
	}
	case CompressedImageColourFormat::colourFull: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		size_t rawSize = bitmapData.PixelFormat == PixelFormat24bppRGB ? 3 : 2;
		for (int row = 0; row < bitmapData.Height; row++) {
			rawDataStream.pushBytes(static_cast<uint8_t*>(bitmapData.Scan0) + row * bitmapData.Stride, rawSize * bitmapData.Width);
		}
		break;
	}
	case CompressedImageColourFormat::packedColour3Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				ConvertibleColour::colour24_t pixel = (static_cast<ConvertibleColour::colour24_t*>(bitmapData.Scan0) + y * bitmapData.Stride)[x];
				ConvertibleColour::colour3_t colour = ConvertibleColour().fromColour24Bit(pixel)->toColour3Bit();
				rawDataStream.pushBits(colour, 3);
			}
		}
		break;
	}
	case CompressedImageColourFormat::packedColour6Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		size_t rawSize = bitmapData.PixelFormat == PixelFormat24bppRGB ? 3 : 2;
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				ConvertibleColour::colour24_t pixel = (static_cast<ConvertibleColour::colour24_t*>(bitmapData.Scan0) + y * bitmapData.Stride)[x];
				ConvertibleColour::colour6_t colour = ConvertibleColour().fromColour24Bit(pixel)->toColour6Bit();
				rawDataStream.pushBits(colour, 6);
			}
		}
		break;
	}
	case CompressedImageColourFormat::packedGreyscale1Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				ConvertibleColour::colour24_t pixel = (static_cast<ConvertibleColour::colour24_t*>(bitmapData.Scan0) + y * bitmapData.Stride)[x];
				ConvertibleColour::greyscale1_t colour = ConvertibleColour().fromColour24Bit(pixel)->toGreyscale1Bit();
				rawDataStream.pushBit(colour);
			}
		}
	}
	case CompressedImageColourFormat::packedGreyscale2Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				ConvertibleColour::colour24_t pixel = (static_cast<ConvertibleColour::colour24_t*>(bitmapData.Scan0) + y * bitmapData.Stride)[x];
				ConvertibleColour::greyscale2_t colour = ConvertibleColour().fromColour24Bit(pixel)->toGreyscale2Bit();
				rawDataStream.pushBits(colour, 2);
			}
		}
	}
	case CompressedImageColourFormat::packedGreyscale3Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				ConvertibleColour::colour24_t pixel = (static_cast<ConvertibleColour::colour24_t*>(bitmapData.Scan0) + y * bitmapData.Stride)[x];
				ConvertibleColour::greyscale3_t colour = ConvertibleColour().fromColour24Bit(pixel)->toGreyscale3Bit();
				rawDataStream.pushBits(colour, 3);
			}
		}
	}
	case CompressedImageColourFormat::packedGreyscale4Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				ConvertibleColour::colour24_t pixel = (static_cast<ConvertibleColour::colour24_t*>(bitmapData.Scan0) + y * bitmapData.Stride)[x];
				ConvertibleColour::greyscale4_t colour = ConvertibleColour().fromColour24Bit(pixel)->toGreyscale4Bit();
				rawDataStream.pushBits(colour, 4);
			}
		}
	}
	case CompressedImageColourFormat::packedIndexBit: {
		extractedPalette = makeSmallOptimalPalette(2, *bitmap, false);
		bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, extractedPalette.get(), 0);
		outputPalette = makeOutputPalette(extractedPalette, paletteFormatDesired);
		break;
	}
	case CompressedImageColourFormat::packedIndex2Bit: {
		extractedPalette = makeSmallOptimalPalette(4, *bitmap, false);
		bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, extractedPalette.get(), 0);
		outputPalette = makeOutputPalette(extractedPalette, paletteFormatDesired);
		break;
	}
	case CompressedImageColourFormat::packedIndex4Bit: {
		extractedPalette = makeSmallOptimalPalette(16, *bitmap, false);
		bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, extractedPalette.get(), 0);
		outputPalette = makeOutputPalette(extractedPalette, paletteFormatDesired);
		break;
	}
	case CompressedImageColourFormat::index8Bit: {
		extractedPalette = makeSmallOptimalPalette(256, *bitmap, false);
		bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, extractedPalette.get(), 0);
		outputPalette = makeOutputPalette(extractedPalette, paletteFormatDesired);
		break;
	}
	}

	return 0;
}

