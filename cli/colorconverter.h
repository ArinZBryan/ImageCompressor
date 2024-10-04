#pragma once
#include <gdiplus.h>
#include <stdint.h>

class ConvertibleColour {
private:
	float red = 0;
	float green = 0;
	float blue = 0;
public:
	using colour555_t = uint16_t;
	using colour565_t = uint16_t;
	static struct colour24_t { uint8_t R; uint8_t G; uint8_t B; };
	using colour3_t = uint8_t;
	using colour6_t = uint8_t;
	using greyscale1_t = uint8_t;
	using greyscale2_t = uint8_t;
	using greyscale3_t = uint8_t;
	using greyscale4_t = uint8_t;
	ConvertibleColour() {}
	ConvertibleColour(float R, float G, float B);
	ConvertibleColour* fromColour555(colour555_t colour);
	ConvertibleColour* fromColour565(colour565_t colour);
	ConvertibleColour* fromColour24Bit(colour24_t colour);
	ConvertibleColour* fromColour3Bytes(uint8_t R, uint8_t G, uint8_t B);
	ConvertibleColour* fromColour3Bit(colour3_t colour);
	ConvertibleColour* fromColour6Bit(colour6_t colour);
	ConvertibleColour* fromColourARGB(Gdiplus::ARGB colour);
	ConvertibleColour* fromGreyscale2Bit(greyscale2_t value);
	ConvertibleColour* fromGreyscale3Bit(greyscale3_t value);
	ConvertibleColour* fromGreyscale4Bit(greyscale4_t value);
	colour555_t toColour555();
	colour565_t toColour565();
	colour24_t toColour24Bit();
	colour3_t toColour3Bit();
	colour6_t toColour6Bit();
	Gdiplus::ARGB toColourARGB();
	greyscale1_t toGreyscale1Bit();
	greyscale2_t toGreyscale2Bit();
	greyscale3_t toGreyscale3Bit();
	greyscale4_t toGreyscale4Bit();
};