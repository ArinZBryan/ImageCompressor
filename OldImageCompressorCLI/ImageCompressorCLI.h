#pragma once
#include <stdint.h>

enum class CompressedImageColourFormat {
	packedIndexBit = 0x1,
	packedIndex2Bit = 0x2,
	packedIndex4Bit = 0x4,
	index8Bit = 0x8,
	packedGreyscale1Bit = 0x10,
	packedGreyscale2Bit = 0x20,
	packedColour3Bit = 0x40,
	packedGreyscale3Bit = 0x80,
	packedGreyscale4Bit = 0x100,
	packedColour6Bit = 0x200,
	colour555 = 0x400,
	colour565 = 0x800,
	colourFull = 0x1000
};

enum class CompressedImagePaletteFormat {
	noPalette = 0x0,
	greyscale2Bit = 0x1,
	colour3Bit = 0x2,
	greyscale3Bit = 0x4,
	greyscale4Bit = 0x8,
	colour6Bit = 0x10,
	colour555 = 0x20,
	colour565 = 0x40,
	colourFull = 0x80
};

struct CompressedImage {
	uint32_t imageSize;
	uint16_t width;
	uint16_t height;
	uint32_t imageDataSizeBytes;
	CompressedImageColourFormat colourFormat;
	uint8_t	rleLength;
	uint8_t paletteSize;
	uint16_t paletteSizeBytes;
	CompressedImagePaletteFormat paletteColourFormat;
	void* palette;
	void* imageData;
};