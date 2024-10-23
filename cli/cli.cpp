#define GDIPVER 0x0110
#include "wingdiputils.h"
#pragma comment (lib,"Gdiplus.lib")
#include <fstream>
#include <iostream>
#include <set>
#include <bitset>
#include "libCLI.h"
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
	"C:\\dev\\ImageCompressor\\Tests\\small\\small_greyscale_24bpp.bmp",
	"-c",
	"pi4",
	"-p",
	"c24",
	"-d",
	"C:\\dev\\ImageCompressor\\Tests\\small\\small_greyscale_24bpp.rlei"
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


class bitvector : public std::vector<bool> {
public:
	using vector::vector;
	template<size_t size>
	void push_many_back(std::bitset<size> bs) {
		for (int i = size - 1; i >= 0; i--) {
			this->push_back(bs[i]);
		}
	}
	void push_many_back(bitvector bv) {
		this->insert(this->end(), bv.begin(), bv.end());
	}
	void push_many_back(uint64_t src, uint32_t count) {
		if (count > 64) { count = 64; }
		for (int i = count - 1; i >= 0; i--) {
			this->push_back(src >> i & 0x1);
		}
	}
	uint8_t* dump(uint8_t* out, size_t size) {
		size_t dumpSize = ((size * 8) > this->size()) ? (this->size()) : (size * 8);
		if (dumpSize <= 0) {
			return nullptr;
		}
		uint8_t buf = 0x0;
		size_t bufUsage = 0;
		size_t bitPos = 0;
		size_t bytePos = 0;
		for (bitPos = 0; bitPos <= dumpSize - 1; bitPos++) {
			buf <<= 1;
			buf |= this->at(bitPos);
			bufUsage++;
			if (bufUsage >= 8) {
				out[bytePos] = buf;
				buf = 0;
				bufUsage = 0;
			}
		}
		if (bufUsage > 0) {
			out[bytePos] = buf << 8 - bufUsage;
		}
		return out;
	}
	std::vector<uint8_t> dump() {
		std::vector<uint8_t> out = std::vector<uint8_t>(0);
		if (this->size() == 0) { return out; }
		uint8_t buf = 0x0;
		size_t bufUsage = 0;
		size_t bitPos = 0;
		size_t bytePos = 0;
		for (bitPos = 0; bitPos <= this->size() - 1; bitPos++) {
			buf <<= 1;
			buf |= this->at(bitPos);
			bufUsage++;
			if (bufUsage >= 8) {
				out.push_back(buf);
				buf = 0;
				bufUsage = 0;
			}
		}
		if (bufUsage > 0) {
			out[bytePos] = buf << 8 - bufUsage;
		}
		return out;
	}
	size_t byte_size() {
		return ceilf(this->size() / 8.0);
	}
};

template<typename T>
void printVector(std::vector<T> vec, int width) {
	for (int i = 0; i < vec.size(); i++) {
		if (i % width == 0 && i != 0) { std::cout << std::endl; }
		std::cout << vec[i] << ' ';
	}
}
template<typename T>
void printVectorInt(std::vector<T> vec, int width) {
	for (int i = 0; i < vec.size(); i++) {
		if (i % width == 0 && i != 0) { std::cout << std::endl; }
		std::cout << +vec[i] << ' ';
	}
}

bitvector runLengthEncode(bitvector& bits, int unitLength, int packLength) {
	if (packLength < unitLength) { return bitvector(0); }
	size_t packingSpace = packLength - unitLength;
	int maxRLEValue = exp2(packingSpace) - 1;
	bitvector res;
	auto bitsBegin = bits.cbegin();
	bitvector run = bitvector(bitsBegin, bitsBegin + unitLength);
	int length = 0;


	bitvector slice = bitvector(unitLength);
	for (int i = 0; i < bits.size(); i += unitLength) {
		slice = bitvector(bitsBegin + i, bitsBegin + i + unitLength);
		if (run == slice) { length++; continue; }
		else {
			while (length > maxRLEValue) {
				for (int packBit = 0; packBit < packingSpace; packBit++) {
					run.push_back(1);
				}
				res.insert(res.end(), run.begin(), run.end());
				length -= maxRLEValue;
			}
			run.push_many_back(length, packingSpace);
			res.insert(res.end(), run.begin(), run.end());
			length = 1;

			run = slice;
		}
	}
	while (length > maxRLEValue) {
		for (int packBit = 0; packBit < packingSpace; packBit++) {
			run.push_back(1);
		}
		res.insert(res.end(), run.begin(), run.end());
		length -= maxRLEValue;
	}
	run.push_many_back(length, packingSpace);
	res.insert(res.end(), run.begin(), run.end());
	return res;
}
bitvector runLengthDecode(bitvector& bits, int unitLength, int packLength) {
	if (packLength < unitLength) { return bitvector(0); }
	auto bitsBegin = bits.cbegin();
	bitvector decoded = bitvector();
	uint8_t* repeatsMemory = nullptr;
	size_t repeatsMemorySize = 0;
	for (int i = 0; i < bits.size(); i += packLength) {
		bitvector packed = bitvector(bitsBegin + i, bitsBegin + i + packLength);
		bitvector value = bitvector(packed.begin(), packed.begin() + unitLength);
		bitvector repeats = bitvector(packed.begin() + unitLength, packed.begin() + packLength);
		if (repeatsMemory == nullptr || repeatsMemorySize == 0) {
			repeatsMemorySize = ceilf(repeats.size() / 8.0);
			repeatsMemory = static_cast<uint8_t*>(malloc(repeatsMemorySize));
		}
		repeats.dump(repeatsMemory, repeatsMemorySize);
		if (repeatsMemorySize == 1) {
			for (int repeatNo = 0; repeatNo < *(reinterpret_cast<uint8_t*>(repeatsMemory)); repeatNo++) {
				decoded.push_many_back(value);
			}
		}
		else if (repeatsMemorySize == 2) {
			for (int repeatNo = 0; repeatNo < *(reinterpret_cast<uint16_t*>(repeatsMemory)); repeatNo++) {
				decoded.push_many_back(value);
			}
		}
		else if (repeatsMemorySize == 4) {
			for (int repeatNo = 0; repeatNo < *(reinterpret_cast<uint32_t*>(repeatsMemory)); repeatNo++) {
				decoded.push_many_back(value);
			}
		}
	}
	free(repeatsMemory);
	return decoded;
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

	image.UnlockBits(&bitmapData);

	if (uniqueColors.size() < 1 || maxSize < 1) {
		return nullptr;
	}
	auto palette = allocatePalette(static_cast<int>(min(maxSize, uniqueColors.size())), imageIsGreyscale ? gdip::PaletteFlagsGrayScale : 0);

	gdip::Bitmap::InitializePalette(palette.get(), gdip::PaletteTypeOptimal, min(maxSize, uniqueColors.size()), false, &image);
	
	return palette;
}
bitvector makeOutputPalette(std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter>& inputPalette, CompressedImagePaletteFormat paletteFormat) {
	bitvector palette = bitvector();
	switch (paletteFormat) {
	case CompressedImagePaletteFormat::noPalette: {
		std::cerr << "[Error] Tried to make palette with format of 'No Palette'" << std::endl;
		return bitvector(0);
		break;
	}
	case CompressedImagePaletteFormat::greyscale2Bit: {
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.push_many_back(std::bitset<2>(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toGreyscale2Bit()));
		}
		break;
	}
	case CompressedImagePaletteFormat::greyscale3Bit: {
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.push_many_back(std::bitset<3>(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toGreyscale3Bit()));
		}
		break;
	}
	case CompressedImagePaletteFormat::greyscale4Bit: {
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.push_many_back(std::bitset<4>(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toGreyscale4Bit()));
		}
		break;
	}
	case CompressedImagePaletteFormat::colour3Bit: {
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.push_many_back(std::bitset<3>(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour3Bit()));
		}
		break;
	}
	case CompressedImagePaletteFormat::colour6Bit: {
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.push_many_back(std::bitset<6>(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour6Bit()));
		}
		break;
	}
	case CompressedImagePaletteFormat::colour555: {
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.push_many_back(std::bitset<16>(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour555()));
		}
		break;
	}
	case CompressedImagePaletteFormat::colour565: {
		for (int i = 0; i < inputPalette->Count; i++) {
			palette.push_many_back(std::bitset<16>(ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour565()));
		}
		break;
	}
	case CompressedImagePaletteFormat::colourFull: {
		for (int i = 0; i < inputPalette->Count; i++) {
			ConvertibleColour::colour24_t col = ConvertibleColour().fromColourARGB(inputPalette->Entries[i])->toColour24Bit();
			palette.push_many_back(std::bitset<8>(col.R));
			palette.push_many_back(std::bitset<8>(col.G));
			palette.push_many_back(std::bitset<8>(col.B));
		}
		break;
	}
	}
	return palette;
}
std::set<bitvector> makePaletteLUT(bitvector palette, CompressedImagePaletteFormat format) {
	std::set<bitvector> LUT;
	auto paletteIter = palette.cbegin();
	size_t formatSize = 0;
	switch (format) {
	case CompressedImagePaletteFormat::greyscale2Bit: formatSize = 2; break;
	case CompressedImagePaletteFormat::greyscale3Bit: formatSize = 3; break;
	case CompressedImagePaletteFormat::greyscale4Bit: formatSize = 4; break;
	case CompressedImagePaletteFormat::colour3Bit: formatSize = 3; break;
	case CompressedImagePaletteFormat::colour6Bit: formatSize = 6; break;
	case CompressedImagePaletteFormat::colour555: formatSize = 16; break;
	case CompressedImagePaletteFormat::colour565: formatSize = 16; break;
	case CompressedImagePaletteFormat::colourFull: formatSize = 24; break;
	}
	while (paletteIter + formatSize < palette.end()) {
		bitvector slice = bitvector(paletteIter, paletteIter + formatSize);
		LUT.insert(slice);
		paletteIter += formatSize;
	}
	return LUT;
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
		uint8_t* dstRow = startPoint + (bitmapData.Stride * i);
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

	CLIArg colourFormat = cliArgs.at("--colour-format");
	std::string colourFormatString;
	uint8_t packedLength = 0;
	uint8_t unitLength = 0;
	uint8_t paletteSize = 0;
	if (!getFromVariantOptional(colourFormat.value, &colourFormatString)) {
		std::cerr << "[Error] Invalid Colour Format" << std::endl;
		return 1;
	}
	CompressedImageColourFormat colourFormatDesired;
	if (colourFormatString == "pi1") { colourFormatDesired = CompressedImageColourFormat::packedIndexBit; packedLength = 8; unitLength = 1;  paletteSize = 2; }
	else if (colourFormatString == "pi2") { colourFormatDesired = CompressedImageColourFormat::packedIndex2Bit; packedLength = 8; unitLength = 2;  paletteSize = 4; }
	else if (colourFormatString == "pi4") { colourFormatDesired = CompressedImageColourFormat::packedIndex4Bit; packedLength = 8; unitLength = 4;  paletteSize = 16; }
	else if (colourFormatString == "i8r1") { colourFormatDesired = CompressedImageColourFormat::index8Bit; packedLength = 16; unitLength = 8;  paletteSize = 256; }
	else if (colourFormatString == "i8r2") { colourFormatDesired = CompressedImageColourFormat::index8Bit; packedLength = 24; unitLength = 8;  paletteSize = 256; }
	else if (colourFormatString == "pg1") { colourFormatDesired = CompressedImageColourFormat::packedGreyscale1Bit; packedLength = 8; unitLength = 1; paletteSize = 0; }
	else if (colourFormatString == "pg2") { colourFormatDesired = CompressedImageColourFormat::packedGreyscale2Bit; packedLength = 8; unitLength = 2; paletteSize = 0; }
	else if (colourFormatString == "pc3") { colourFormatDesired = CompressedImageColourFormat::packedColour3Bit; packedLength = 8; unitLength = 3;  paletteSize = 0; }
	else if (colourFormatString == "pg3") { colourFormatDesired = CompressedImageColourFormat::packedGreyscale3Bit; packedLength = 8; unitLength = 3; paletteSize = 0; }
	else if (colourFormatString == "pg4") { colourFormatDesired = CompressedImageColourFormat::packedGreyscale4Bit; packedLength = 8; unitLength = 4; paletteSize = 0; }
	else if (colourFormatString == "pc6") { colourFormatDesired = CompressedImageColourFormat::packedColour6Bit; packedLength = 8; unitLength = 6;  paletteSize = 0; }
	else if (colourFormatString == "c555r1") { colourFormatDesired = CompressedImageColourFormat::colour555; packedLength = 24; unitLength = 16;  paletteSize = 0; }
	else if (colourFormatString == "c555r2") { colourFormatDesired = CompressedImageColourFormat::colour555; packedLength = 32; unitLength = 16;  paletteSize = 0; }
	else if (colourFormatString == "c565r1") { colourFormatDesired = CompressedImageColourFormat::colour565; packedLength = 24; unitLength = 16;  paletteSize = 0; }
	else if (colourFormatString == "c565r2") { colourFormatDesired = CompressedImageColourFormat::colour565; packedLength = 32; unitLength = 16;  paletteSize = 0; }
	else if (colourFormatString == "c24r1") { colourFormatDesired = CompressedImageColourFormat::colourFull; packedLength = 32; unitLength = 24;  paletteSize = 0; }
	else if (colourFormatString == "c24r2") { colourFormatDesired = CompressedImageColourFormat::colourFull; packedLength = 40; unitLength = 24;  paletteSize = 0; }
	else {
		colourFormatDesired = CompressedImageColourFormat::colour565;
		std::cout << "[Info] No colour format supplied, using 16-bit 565 colour, with a run-length of 1" << std::endl;
	}

	bitvector rawDataStream;
	bitvector rledDataStream;

	CLIArg paletteFormatArg;
	CompressedImagePaletteFormat paletteFormatDesired;
	if (cliArgs.contains("--palette-format")) {
		paletteFormatArg = cliArgs.at("--palette-format");
		std::string paletteFormatString;
		if (!getFromVariantOptional(paletteFormatArg.value, &paletteFormatString)) {
			std::cerr << "[Error] Invalid Colour Format" << std::endl;
			return 1;
		}
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
	}
	else {
		paletteFormatDesired = CompressedImagePaletteFormat::noPalette;
	}

	bitvector outputPalette = bitvector();
	switch (colourFormatDesired) {
	case CompressedImageColourFormat::colour555: {
		bitmap->ConvertFormat(PixelFormat16bppRGB555, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				rsize_t pixel_start_offset = x * 2 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				rawDataStream.push_many_back(pixel_start[0], 8);
				rawDataStream.push_many_back(pixel_start[1], 8);
			}
		}
		break;
	}
	case CompressedImageColourFormat::colour565: {
		bitmap->ConvertFormat(PixelFormat16bppRGB565, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				size_t pixel_start_offset = x * 2 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				rawDataStream.push_many_back(pixel_start[0], 8);
				rawDataStream.push_many_back(pixel_start[1], 8);
			}
		}
		break;
	}
	case CompressedImageColourFormat::colourFull: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);

		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				size_t pixel_start_offset = x * 3 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				rawDataStream.push_many_back(pixel_start[0], 8);
				rawDataStream.push_many_back(pixel_start[1], 8);
				rawDataStream.push_many_back(pixel_start[2], 8);
			}
		}
		bitmap->UnlockBits(&bitmapData);
		break;
	}
	case CompressedImageColourFormat::packedColour3Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				size_t pixel_start_offset = x * 3 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				ConvertibleColour::colour3_t colour = ConvertibleColour().fromColour24Bit({pixel_start[0], pixel_start[1], pixel_start[3]})->toColour3Bit();
				rawDataStream.push_many_back(std::bitset<3>(colour));
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
				size_t pixel_start_offset = x * 3 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				ConvertibleColour::colour3_t colour = ConvertibleColour().fromColour24Bit({ pixel_start[0], pixel_start[1], pixel_start[3] })->toColour6Bit();
				rawDataStream.push_many_back(std::bitset<6>(colour));
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
				size_t pixel_start_offset = x * 3 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				ConvertibleColour::colour3_t colour = ConvertibleColour().fromColour24Bit({ pixel_start[0], pixel_start[1], pixel_start[3] })->toGreyscale1Bit();
				rawDataStream.push_back(static_cast<bool>(colour));
			}
		}
		break;
	}
	case CompressedImageColourFormat::packedGreyscale2Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				size_t pixel_start_offset = x * 3 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				ConvertibleColour::colour3_t colour = ConvertibleColour().fromColour24Bit({ pixel_start[0], pixel_start[1], pixel_start[3] })->toGreyscale2Bit();
				rawDataStream.push_many_back(std::bitset<2>(colour));
			}
		}
		break;
	}
	case CompressedImageColourFormat::packedGreyscale3Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				size_t pixel_start_offset = x * 3 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				ConvertibleColour::colour3_t colour = ConvertibleColour().fromColour24Bit({ pixel_start[0], pixel_start[1], pixel_start[3] })->toGreyscale3Bit();
				rawDataStream.push_many_back(std::bitset<3>(colour));
			}
		}
		break;
	}
	case CompressedImageColourFormat::packedGreyscale4Bit: {
		bitmap->ConvertFormat(PixelFormat24bppRGB, gdip::DitherTypeNone, gdip::PaletteTypeCustom, nullptr, 0);
		gdip::BitmapData bitmapData;
		gdip::Rect rect(0, 0, bitmap->GetWidth(), bitmap->GetHeight());
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				size_t pixel_start_offset = x * 3 + y * bitmapData.Stride;
				uint8_t* pixel_start = static_cast<uint8_t*>(bitmapData.Scan0) + pixel_start_offset;
				ConvertibleColour::colour3_t colour = ConvertibleColour().fromColour24Bit({ pixel_start[0], pixel_start[1], pixel_start[3] })->toGreyscale4Bit();
				rawDataStream.push_many_back(std::bitset<4>(colour));
			}
		}
		break;
	}
	case CompressedImageColourFormat::packedIndexBit: {
		std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter> extractedPalette = makeSmallOptimalPalette(paletteSize, *bitmap, false);
		bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, extractedPalette.get(), 0);
		outputPalette = makeOutputPalette(extractedPalette, paletteFormatDesired);
		std::set<bitvector> paletteLUT = makePaletteLUT(outputPalette, paletteFormatDesired);
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				rawDataStream.push_back(*(static_cast<uint8_t*>(bitmapData.Scan0) + y * bitmapData.Stride + x));
			}
		}
		break;
	}
	case CompressedImageColourFormat::packedIndex2Bit: {
		std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter> extractedPalette = makeSmallOptimalPalette(paletteSize, *bitmap, false);
		bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, extractedPalette.get(), 0);
		outputPalette = makeOutputPalette(extractedPalette, paletteFormatDesired);
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				rawDataStream.push_many_back(*(static_cast<uint8_t*>(bitmapData.Scan0) + y * bitmapData.Stride + x),2);
			}
		}
		break;
	}
	case CompressedImageColourFormat::packedIndex4Bit: {
		std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter> extractedPalette = makeSmallOptimalPalette(paletteSize, *bitmap, false);
		outputPalette = makeOutputPalette(extractedPalette, paletteFormatDesired);
		
		//I've fixed the palette issues, but now the same issue crops up again here. Even though the palettes are
		//correct, bitmap->ConvertFormat seems to just be being stupid here. When we use gdip::DitherTypeNone
		//It doesn't convert the format at all, and when we use gdip::DitherTypeSolid, it produces the output file
		//With rounding. 

		//TODO: Copy the old check for same-size palettes from makeSmallOptimalPalette to here. If it is true,
		//		The use makePaletteLUT to make a set of ARGB colours, and manually perform the format conversion.
		//		-> makePaletteLUT (or simmilar, maybe modify the function) on extractedPalette
		//		-> convert to ARGB Format (via gdip)
		//		-> convert the image to indexed colour manually using the resultant set
		//		-> convert the palette to the required format (makeOutputPalette)
		bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, extractedPalette.get(), 0);
		bitmap->LockBits(&rect, gdip::ImageLockModeRead, bitmap->GetPixelFormat(), &bitmapData);

		std::cout << "Small Optimal Palette Values: " << std::hex;
		for (int i = 0; i < extractedPalette.get()->Count; i++) {
			std::cout << extractedPalette.get()->Entries[i] << " ";
		}

		std::cout << std::endl << "Output Palette: ";
		auto tmp = outputPalette.dump();
		for (int i = 0; i < tmp.size(); i++) {
			std::cout << static_cast<int>(tmp[i]) << " ";
		}

		std::cout << std::endl << "Image Data: ";
		for (int i = 0; i < bitmapData.Height * bitmapData.Stride; i++) {
			std::cout << static_cast<int>(static_cast<uint8_t*>(bitmapData.Scan0)[i]) << " ";
		}


		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				rawDataStream.push_many_back(*(static_cast<uint8_t*>(bitmapData.Scan0) + y * bitmapData.Stride + x), 4);
			}
		}

		

		break;
	}
	case CompressedImageColourFormat::index8Bit: {
		std::unique_ptr<gdip::ColorPalette, ColorPaletteDeleter> extractedPalette = makeSmallOptimalPalette(paletteSize, *bitmap, false);
		bitmap->ConvertFormat(PixelFormat8bppIndexed, gdip::DitherTypeSolid, gdip::PaletteTypeCustom, extractedPalette.get(), 0);
		outputPalette = makeOutputPalette(extractedPalette, paletteFormatDesired);
		for (int y = 0; y < bitmapData.Height; y++) {
			for (int x = 0; x < bitmapData.Width; x++) {
				rawDataStream.push_many_back(*(static_cast<uint8_t*>(bitmapData.Scan0) + y * bitmapData.Stride + x), 8);
			}
		}
		break;
	}
	}


	rledDataStream = runLengthEncode(rawDataStream, unitLength, packedLength);

	struct CompressedImage finalFile;
	finalFile.identifier[0] = 'R';
	finalFile.identifier[1] = 'L';
	finalFile.identifier[2] = 'E';
	finalFile.identifier[3] = 'I';
	finalFile.version = 1;
	finalFile.imageSize = outputPalette.byte_size() + rledDataStream.byte_size() + sizeof(CompressedImage) - 2 * sizeof(void*);
	finalFile.width = bitmap->GetWidth();
	finalFile.height = bitmap->GetHeight();
	finalFile.imageDataSizeBytes = rledDataStream.byte_size();
	finalFile.colourFormat = colourFormatDesired;
	finalFile.packedLength = packedLength;
	finalFile.unitLength = unitLength;
	finalFile.paletteSize = paletteSize;
	finalFile.padding1 = 0;
	finalFile.paletteSizeBytes = outputPalette.byte_size();
	finalFile.padding2 = 0;
	finalFile.paletteColourFormat = paletteFormatDesired;
	finalFile.padding3 = 0;
	finalFile.palette = nullptr;
	finalFile.imageData = nullptr;

	CLIArg outputFileNameArg = cliArgs.at("--destination");
	std::string outputFileName;
	if (!getFromVariantOptional(outputFileNameArg.value, &outputFileName)) {
		std::cerr << "[Error] File Path Required" << std::endl;
		return 1;
	}
	
	auto outputFile = std::fstream(outputFileName, std::ios::binary | std::ios::out);
	outputFile.write(reinterpret_cast<char*>(&finalFile), sizeof(CompressedImage) - 2 * sizeof(void*));
	auto palOut = outputPalette.dump();
	outputFile.write(reinterpret_cast<const char*>(palOut.data()), palOut.size());
	auto imgOut = rledDataStream.dump();
	outputFile.write(reinterpret_cast<const char*>(imgOut.data()), imgOut.size());
	outputFile.close();

	bitvector().swap(outputPalette);
	bitvector().swap(rledDataStream);

	return 0;
}

