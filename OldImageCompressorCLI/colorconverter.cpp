#include <math.h>
#include "colorconverter.h"

namespace gdip = Gdiplus;

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
	using greyscale2_t = uint8_t;
	using greyscale3_t = uint8_t;
	using greyscale4_t = uint8_t;
	ConvertibleColour() {}
	ConvertibleColour(float R, float G, float B) { red = R; green = G, blue = B; }
	ConvertibleColour* fromColour555(colour555_t colour) {
		red		= ((colour & 0b0111110000000000) >> 10) / 31;
		green	= ((colour & 0b0000001111100000) >> 5) / 31;
		blue	= ((colour & 0b0000000000011111) >> 0) / 31;
		return this;
	}
	ConvertibleColour* fromColour565(colour565_t colour) {
		red		= ((colour & 0b1111100000000000) >> 11) / 31;
		green	= ((colour & 0b0000011111100000) >> 5) / 63;
		blue	= ((colour & 0b0000000000011111) >> 0) / 31;
		return this;
	}
	ConvertibleColour* fromColour24Bit(colour24_t colour) {
		red = colour.R / 255;
		green = colour.G / 255;
		blue = colour.B / 255;
		return this;
	}
	ConvertibleColour* fromColour3Bytes(uint8_t R, uint8_t G, uint8_t B) {
		red		= R / 255;
		green	= G / 255;
		blue	= B / 255;
		return this;
	}
	ConvertibleColour* fromColour3Bit(colour3_t colour) {
		red = (colour & 0b100) ? 1.0 : 0.0;
		green = (colour & 0b010) ? 1.0 : 0.0;
		blue = (colour & 0b001) ? 1.0 : 0.0;
		return this;
	}
	ConvertibleColour* fromColour6Bit(colour6_t colour) {
		red = ((colour & 0b110000) >> 4) / 3;
		green = ((colour & 0b001100) >> 2) / 3;
		blue = ((colour & 0b000011) >> 0) / 3;
		return this;
	}
	ConvertibleColour* fromColourARGB(gdip::ARGB colour) {
		red = (colour >> 16) & 0xFF;	// Extract the Red channel
		green = (colour >> 8) & 0xFF;  // Extract the Green channel
		blue = colour & 0xFF;			// Extract the Blue channel (least significant byte)
		return this;
	}
	ConvertibleColour* fromGreyscale2Bit(greyscale2_t value) {
		red = value / 3;
		green = value / 3;
		blue = value / 3;
		return this;
	}
	ConvertibleColour* fromGreyscale3Bit(greyscale3_t value) {
		red = value / 7;
		green = value / 7;
		blue = value / 7;
		return this;
	}
	ConvertibleColour* fromGreyscale4Bit(greyscale4_t value) {
		red = value / 15;
		green = value / 15;
		blue = value / 15;
		return this;
	}
	colour555_t toColour555() { 
		uint8_t redBits = roundf(red * 31);
		uint8_t greenBits = roundf(green * 31);
		uint8_t blueBits = roundf(blue * 31);
		return redBits << 10 | greenBits << 5 | blueBits;
	}
	colour565_t toColour565() {
		uint8_t redBits = roundf(red * 31);
		uint8_t greenBits = roundf(green * 63);
		uint8_t blueBits = roundf(blue * 31);
		return redBits << 11 | greenBits << 5 | blueBits;
	}
	colour24_t toColour24Bit() {
		uint8_t redBits = roundf(red * 255);
		uint8_t greenBits = roundf(green * 255);
		uint8_t blueBits = roundf(blue * 255);
		return { redBits, greenBits, blueBits };
	}
	colour3_t toColour3Bit() {
		uint8_t redBits = roundf(red);
		uint8_t greenBits = roundf(green);
		uint8_t blueBits = roundf(blue);
		return redBits << 2 | greenBits << 1 | blueBits;
	}
	colour6_t toColour6Bit() {
		uint8_t redBits = roundf(red * 2);
		uint8_t greenBits = roundf(green * 2);
		uint8_t blueBits = roundf(blue * 2);
		return redBits << 4 | greenBits << 2 | blueBits;
	}
	gdip::ARGB toColourARGB() {
		uint8_t redBits = roundf(red * 255);
		uint8_t greenBits = roundf(green * 255);
		uint8_t blueBits = roundf(blue * 255);
		return 0xff << 24 | redBits << 16 | greenBits << 8 | blueBits;
	}
	greyscale2_t toGreyscale2Bit() {
		if (red != green && red != blue) {
			return 0xff;
		}
		return roundf(red * 3);
	}
	greyscale3_t toGreyscale3Bit() {
		if (red != green && red != blue) {
			return 0xff;
		}
		return roundf(red * 7);
	}
	greyscale4_t toGreyscale4Bit() {
		if (red != green && red != blue) {
			return 0xff;
		}
		return roundf(red * 15);
	}
};