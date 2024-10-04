#define GDIPVER 0x0110
#include <iostream>
#include <ostream>
#include <sstream>
#include <set>
#include <windows.h>
#include <gdiplus.h>
#include <gdiplusheaders.h>
#include "libCLI.h"
#include "wingdiputils.h"
#include "ImageCompressorCLI.h"
#include "colorconverter.h"
#include "bitstream.h"
#pragma comment (lib,"Gdiplus.lib")

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

	if (cliArgs.contains("--destination")) {
		//Save BMP copy after any transformations
		std::string outputPath;
		if (!getFromVariantOptional(cliArgs.at("--destination").value, &outputPath)) {
			std::cerr << "[Error] Misformatted Argument: --destination (-d)" << std::endl << "	Expected: File Path Including Extension" << std::endl;
			return 1;
		}
		CLSID encoderCLSID;
		GetEncoderClsid(L"image/bmp", &encoderCLSID);
		std::wstring wideFileDestinationPath = to_wide(outputPath);
		gdip::Status saveResult = bitmap->Save(wideFileDestinationPath.c_str(), &encoderCLSID);
		if (GdiplusErrorHandler(saveResult)) { return 1; }
	}

	//Grab the palette if it exists
	auto palette = allocatePalette(static_cast<size_t>(bitmap->GetPaletteSize()), 0);
	if (bitmap->GetPalette(palette.get(), bitmap->GetPaletteSize()) != gdip::Status::Ok) {
		std::cerr << "[Error] Could Not Get Palette Data" << std::endl; return 1;
	}
	if (palette->Count > 0) {
		std::cout << "[Info] Image Palette Contains " << palette->Count << " Entries" << std::endl;
#ifdef DEBUG
		std::cout << "[Debug] Image Palette Flags: " << std::endl;
		if (palette->Flags & 0b001) { std::cout << "	[/] Palette Contains Alpha Channel Values" << std::endl; }
		else { std::cout << "	[x] Palette Contains Alpha Channel Values" << std::endl; }
		if (palette->Flags & 0b010) { std::cout << "	[/] Palette Is Greyscale" << std::endl; }
		else { std::cout << "	[x] Palette Is Greyscale" << std::endl; }
		if (palette->Flags & 0b100) { std::cout << "	[/] Palette Is Windows Halftone Palette" << std::endl; }
		else { std::cout << "	[x] Palette Is Windows Halftone Palette" << std::endl; }
#endif // DEBUG
		if (palette->Flags & 0b100) { std::cerr << "[Error] Halftone Palette Not Supported" << std::endl; return 1; }
	}
	else {
		int palSize = 256;
		if (cliArgs.contains("--colour-format")) {
			std::string colFmtStr;
			getFromVariantOptional(cliArgs.at("--colour-format").value, &colFmtStr);
			if (colFmtStr == "pi1") { palSize = 2; }
			else if (colFmtStr == "pi2") { palSize = 4; }
			else if (colFmtStr == "pi4") { palSize = 16; }
			else if (colFmtStr == "i8r1") { palSize = 256; }
			else if (colFmtStr == "i8r2") { palSize = 256; }
			else { palSize = 256; }
		}
		bool greyscalePalette = false;
		if (cliArgs.contains("--palette-format")) {
			std::string palFmtStr;
			getFromVariantOptional(cliArgs.at("--palette-format").value, &palFmtStr);
			greyscalePalette = (palFmtStr[0] == 'g');
		}

		palette = makeSmallOptimalPalette(palSize, *bitmap, greyscalePalette);

	}

#ifdef DEBUG
	std::cout << "Palette Colours:" << std::endl;
	for (int i = 0; i < palette->Count; i++) {
		gdip::ARGB col = palette->Entries[i];
		uint8_t alpha = (col >> 24) & 0xFF;  // Extract the Alpha channel (most significant byte)
		uint8_t red = (col >> 16) & 0xFF;	// Extract the Red channel
		uint8_t green = (col >> 8) & 0xFF;  // Extract the Green channel
		uint8_t blue = col &0xFF;			// Extract the Blue channel (least significant byte)
		std::cout << std::hex;
		std::cout << "A" << static_cast<int>(alpha);
		std::cout << "R" << static_cast<int>(red);
		std::cout << "G" << static_cast<int>(green);
		std::cout << "B" << static_cast<int>(blue);
		std::cout << std::dec << std::endl;
	}
#endif // DEBUG

	//Build The Image Struct
	CompressedImage finalImage;
	finalImage.width = widthDesired;
	finalImage.height = heightDesired;
	finalImage.paletteSize = palette->Count;

	//Set settings based on cli args
	CompressedImageColourFormat fmt;
	int rleLen = 1;
	std::string fmtstr;
	if (!cliArgs.contains("--colour-format")) {
		fmt = CompressedImageColourFormat::colour565;
		rleLen = 1;
		std::cout << "[Info] No colour format supplied, using 16-bit 565 colour, with a run-length of 1" << std::endl;
	}
	else {
		getFromVariantOptional(cliArgs.at("--colour-format").value, &fmtstr);
	}
	if (fmtstr == "pi1") { fmt = CompressedImageColourFormat::packedIndexBit; rleLen = 0; }
	else if (fmtstr == "pi2") { fmt = CompressedImageColourFormat::packedIndex2Bit; rleLen = 0; }
	else if (fmtstr == "pi4") { fmt = CompressedImageColourFormat::packedIndex4Bit; rleLen = 0; }
	else if (fmtstr == "i8r1") { fmt = CompressedImageColourFormat::index8Bit; rleLen = 1; }
	else if (fmtstr == "i8r2") { fmt = CompressedImageColourFormat::index8Bit; rleLen = 2; }
	else if (fmtstr == "pg1") { fmt = CompressedImageColourFormat::packedGreyscale1Bit; rleLen = 0; }
	else if (fmtstr == "pg2") { fmt = CompressedImageColourFormat::packedGreyscale2Bit; rleLen = 0; }
	else if (fmtstr == "pc3") { fmt = CompressedImageColourFormat::packedColour3Bit; rleLen = 0; }
	else if (fmtstr == "pg3") { fmt = CompressedImageColourFormat::packedGreyscale3Bit; rleLen = 0; }
	else if (fmtstr == "pg4") { fmt = CompressedImageColourFormat::packedGreyscale4Bit; rleLen = 0; }
	else if (fmtstr == "pc6") { fmt = CompressedImageColourFormat::packedColour6Bit; rleLen = 1; }
	else if (fmtstr == "c555r1") { fmt = CompressedImageColourFormat::colour555; rleLen = 1; }
	else if (fmtstr == "c555r2") { fmt = CompressedImageColourFormat::colour555; rleLen = 2; }
	else if (fmtstr == "c565r1") { fmt = CompressedImageColourFormat::colour565; rleLen = 1; }
	else if (fmtstr == "c565r2") { fmt = CompressedImageColourFormat::colour565; rleLen = 2; }
	else if (fmtstr == "c24r1") { fmt = CompressedImageColourFormat::colourFull; rleLen = 1; }
	else if (fmtstr == "c24r2") { fmt = CompressedImageColourFormat::colourFull; rleLen = 2; }
	else {
		fmt = CompressedImageColourFormat::colour565;
		rleLen = 0;
		std::cout << "[Info] No colour format supplied, using 16-bit 565 colour, with a run-length of 1" << std::endl;
	}
	finalImage.colourFormat = fmt;
	finalImage.rleLength = rleLen;

	//build the palette and convert image to indecies first if applicable
	switch (fmt)
	{
		case CompressedImageColourFormat::packedIndexBit:
		case CompressedImageColourFormat::packedIndex2Bit:
		case CompressedImageColourFormat::packedIndex4Bit:
		case CompressedImageColourFormat::index8Bit:
		{
			if (!cliArgs.contains("--palette-format")) {
				std::cout << "[Error] When using indexed colour formats, --palette-format must be supplied" << std::endl;
				return 1;
			}
			std::string palFmtStr;
			getFromVariantOptional(cliArgs.at("--palette-format").value, &palFmtStr);
			if (palFmtStr == "g2") { 
				finalImage.palette = calloc(finalImage.paletteSize, sizeof(ConvertibleColour::greyscale2_t));
				finalImage.paletteSizeBytes = finalImage.paletteSize * sizeof(ConvertibleColour::greyscale2_t);
				finalImage.paletteColourFormat = CompressedImagePaletteFormat::greyscale2Bit;
				for (int i = 0; i < finalImage.paletteSize; i++) {
					static_cast<ConvertibleColour::greyscale2_t*>(finalImage.palette)[i] = ConvertibleColour().fromColourARGB(palette->Entries[i])->toGreyscale2Bit();
				}
			} 
			else if (palFmtStr == "c3") { 
				finalImage.palette = calloc(finalImage.paletteSize, sizeof(ConvertibleColour::colour3_t));
				finalImage.paletteSizeBytes = finalImage.paletteSize * sizeof(ConvertibleColour::colour3_t);
				finalImage.paletteColourFormat = CompressedImagePaletteFormat::colour3_t;
				for (int i = 0; i < finalImage.paletteSize; i++) {
					static_cast<ConvertibleColour::colour3_t*>(finalImage.palette)[i] = ConvertibleColour().fromColourARGB(palette->Entries[i])->toColour3Bit();
				}
			} 
			else if (palFmtStr == "g3") { 
				finalImage.palette = calloc(finalImage.paletteSize, sizeof(ConvertibleColour::greyscale3_t));
				finalImage.paletteSizeBytes = finalImage.paletteSize * sizeof(ConvertibleColour::greyscale3_t);
				finalImage.paletteColourFormat = CompressedImagePaletteFormat::greyscale3Bit;
				for (int i = 0; i < finalImage.paletteSize; i++) {
					static_cast<ConvertibleColour::greyscale3_t*>(finalImage.palette)[i] = ConvertibleColour().fromColourARGB(palette->Entries[i])->toGreyscale3Bit();
				}
			} 
			else if (palFmtStr == "g4") { 
				finalImage.palette = calloc(finalImage.paletteSize, sizeof(ConvertibleColour::greyscale4_t));
				finalImage.paletteSizeBytes = finalImage.paletteSize * sizeof(ConvertibleColour::greyscale4_t);
				finalImage.paletteColourFormat = CompressedImagePaletteFormat::greyscale4Bit;
				for (int i = 0; i < finalImage.paletteSize; i++) {
					static_cast<ConvertibleColour::greyscale4_t*>(finalImage.palette)[i] = ConvertibleColour().fromColourARGB(palette->Entries[i])->toGreyscale4Bit();
				}
			} 
			else if (palFmtStr == "c6") { 
				finalImage.palette = calloc(finalImage.paletteSize, sizeof(ConvertibleColour::colour6_t));
				finalImage.paletteSizeBytes = finalImage.paletteSize * sizeof(ConvertibleColour::colour6_t);
				finalImage.paletteColourFormat = CompressedImagePaletteFormat::colour6Bit;
				for (int i = 0; i < finalImage.paletteSize; i++) {
					static_cast<ConvertibleColour::colour6_t*>(finalImage.palette)[i] = ConvertibleColour().fromColourARGB(palette->Entries[i])->toColour6Bit();
				}
			} 
			else if (palFmtStr == "c555") { 
				finalImage.palette = calloc(finalImage.paletteSize, sizeof(ConvertibleColour::colour555_t));
				finalImage.paletteSizeBytes = finalImage.paletteSize * sizeof(ConvertibleColour::colour555_t);
				finalImage.paletteColourFormat = CompressedImagePaletteFormat::colour555;
				for (int i = 0; i < finalImage.paletteSize; i++) {
					static_cast<ConvertibleColour::colour555_t*>(finalImage.palette)[i] = ConvertibleColour().fromColourARGB(palette->Entries[i])->toColour555();
				}
			} 
			else if (palFmtStr == "c565") { 
				finalImage.palette = calloc(finalImage.paletteSize, sizeof(ConvertibleColour::colour565_t));
				finalImage.paletteSizeBytes = finalImage.paletteSize * sizeof(ConvertibleColour::colour565_t);
				finalImage.paletteColourFormat = CompressedImagePaletteFormat::colour565;
				for (int i = 0; i < finalImage.paletteSize; i++) {
					static_cast<ConvertibleColour::colour565_t*>(finalImage.palette)[i] = ConvertibleColour().fromColourARGB(palette->Entries[i])->toColour565();
				}
			} 
			else if (palFmtStr == "c24") { 
				finalImage.palette = calloc(finalImage.paletteSize, sizeof(ConvertibleColour::colour24_t));
				finalImage.paletteSizeBytes = finalImage.paletteSize * sizeof(ConvertibleColour::colour24_t);
				finalImage.paletteColourFormat = CompressedImagePaletteFormat::colourFull;
				for (int i = 0; i < finalImage.paletteSize; i++) {
					static_cast<ConvertibleColour::colour24_t*>(finalImage.palette)[i] = ConvertibleColour().fromColourARGB(palette->Entries[i])->toColour24Bit();
				}
			}
			
			//Lock bitmap for reading
			gdip::BitmapData bitmapData;
			gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
			bitmap->LockBits(&rect, gdip::ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);
			
			size_t indexBufSize = bitmap->GetWidth() * bitmap->GetHeight();
			if (fmt == CompressedImageColourFormat::packedIndexBit) indexBufSize << 3;
			if (fmt == CompressedImageColourFormat::packedIndex2Bit) indexBufSize << 2;
			if (fmt == CompressedImageColourFormat::packedIndex4Bit) indexBufSize << 1;

			int indexWidth;
			switch (bitmapData.PixelFormat) {
			case PixelFormat1bppIndexed: indexWidth = 1; break;
			case PixelFormat4bppIndexed: indexWidth = 4; break;
			case PixelFormat8bppIndexed: indexWidth = 8; break;
			default:
				//Index the image
				for ()
				break;
			}

			for (int y = 0; y < bitmapData.Height; y++) {
				for (int indexStartBit = 0; indexStartBit < bitmapData.Width * indexWidth; indexStartBit++) {
				}
			}

			break;
		}
		default:
		{
			finalImage.palette = nullptr;
			finalImage.paletteSizeBytes = 0;
			finalImage.paletteColourFormat = CompressedImagePaletteFormat::noPalette;
			break;
		}
			
	}

	return 0;
}

