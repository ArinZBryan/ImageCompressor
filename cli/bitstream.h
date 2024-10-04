#pragma once
#include <deque>
#include <stdint.h>
#include <memory>

class bitstream {
private:
	std::deque<uint8_t> buf;
	uint8_t allocation_buf;
	uint8_t allocation_buf_usage;
public:
	bitstream();
	uint8_t flushBuffer();
	bitstream* pushBit(bool bit);
	bitstream* pushBits(uint32_t source, int bits);
	bitstream* pushBytes(std::deque<uint8_t>& bytes);
	bitstream* pushBytes(std::deque<uint16_t>& words);
	bitstream* pushBytes(std::deque<uint32_t>& dwords);
	bitstream* pushBytes(uint8_t* source, int bytes);
	bitstream* pushBytes(bitstream& bytes);
	bool popBit();
	uint32_t popBits(int bits);
	const std::vector<uint8_t>& getHeapBytes();
	uint8_t getBufferByte();
	uint8_t getBufferUsage();
	int getAllBits(std::deque<uint8_t>& bytes);
	uint8_t* getBytes(uint8_t* buf, int buflen);
	int bitLength();
	int byteLength();
};