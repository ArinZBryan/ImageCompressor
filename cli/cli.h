#pragma once
#include <stdint.h>

#define abs(x) (x < 0 ? -x : x)
#define sign(x) (x < 0 ? -1 : (x > 0 ? 1 : 0))

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
	char identifier[4];
	uint32_t version;
	uint32_t imageSize;
	uint16_t width;
	uint16_t height;
	uint32_t imageDataSizeBytes;
	CompressedImageColourFormat colourFormat;
	uint8_t	unitLength;
	uint8_t packedLength;
	uint8_t paletteSize;
	uint8_t padding1;
	uint16_t paletteSizeBytes;
	uint16_t padding2;
	CompressedImagePaletteFormat paletteColourFormat;
	void* palette;
	void* imageData;
};