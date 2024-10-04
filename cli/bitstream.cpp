#include "bitstream.h"

class bitstream {
private:
	std::deque<uint8_t> buf;
	uint8_t allocation_buf;
	uint8_t allocation_buf_usage;
	uint8_t take_buf_usage;
	uint8_t take_buf;
public:
	bitstream() {
		buf = std::vector<uint8_t>();
		allocation_buf = 0x0;
		allocation_buf_usage = 0;
	}
	uint8_t flushBuffer() {
		buf.push_back(allocation_buf);
		allocation_buf = 0x0;
		if (take_buf_usage == 0) {
			take_buf = buf.front();
			buf.pop_front();
			take_buf_usage = 8;
		}
		return allocation_buf_usage;
	}
	bitstream* pushBit(bool bit) {
		if (allocation_buf_usage >= 8) { flushBuffer(); }
		allocation_buf <<= 1;
		allocation_buf |= bit;
		allocation_buf_usage++;
		return this;
	}
	bitstream* pushBits(uint32_t source, int bits) {
		if (bits > 32) {
			bits = 32;
		}
		for (int i = 0; i < bits; i++) {
			pushBit(source & 0x80000000 >> i);
		}
		return this;
	}
	bitstream* pushByte(uint8_t byte) {
		if (allocation_buf_usage == 0) { allocation_buf = byte; flushBuffer(); }
		else { pushBits(byte, 8); }
		return this;
	}
	bitstream* pushBytes(std::deque<uint8_t>& bytes) {
		for (int i = 0; i < bytes.size(); i++) {
			pushBits(static_cast<uint32_t>(bytes[i]), 8);
		}
		return this;
	}
	bitstream* pushBytes(std::deque<uint16_t>& words) {
		for (int i = 0; i < words.size(); i++) {
			pushBits(static_cast<uint32_t>(words[i]), 16);
		}
		return this;
	}
	bitstream* pushBytes(std::deque<uint32_t>& dwords) {
		for (int i = 0; i < dwords.size(); i++) {
			pushBits(dwords[i], 32);
		}
		return this;
	}
	bitstream* pushBytes(uint8_t* source, int bytes) {
		for (int i = 0; i < bytes; i++) {
			pushByte(source[i]);
		}
		return this;
	}
	bitstream* pushBytes(bitstream& bytes) {
		pushBits(bytes.getBufferByte(), bytes.getBufferUsage());
		std::deque<uint8_t> underlying_bytes = bytes.getHeapBytes();
		return pushBytes(underlying_bytes);
	}
	bool popBit() {
		if (allocation_buf_usage != 0) {
			allocation_buf_usage--;
			return static_cast<bool>((allocation_buf >> 8) & 0x1);
		}
		else {
			return static_cast<bool>(buf.back() & 0x1);
		}
	}
	uint32_t popBits(int bits) {
		if (bits > 32) {
			bits = 32;
		}
		uint32_t ret = 0;
		for (int i = 0; i < bits; i++) {
			ret |= popBit();
			ret <<= 1;
		}
		return ret;
	}
	bool takeBit() {
		if (take_buf_usage == 8) {
			take_buf = buf.front();
			buf.pop_front();
		}

	}
	const std::deque<uint8_t>& getHeapBytes() {
		return buf;
	}
	uint8_t getBufferByte() { return allocation_buf; }
	uint8_t getBufferUsage() { return allocation_buf_usage; }
	int getAllBits(std::deque<uint8_t>& bytes) {
		bytes = buf;
		bytes.push_back(allocation_buf);
		return buf.size() + allocation_buf_usage;
	}
	uint8_t* getBytes(uint8_t* buf, int buflen) {
		if (buflen <= this->buf.size()) {
			for (int i = 0; i < buflen; i++) { buf[i] = this->buf[i]; }
		}
		else {
			for (int i = 0; i < buflen; i++) { buf[i] = this->buf[i]; }
			buf[this->buf.size()] = allocation_buf;
		}
		return buf;
	}
	int bitLength() { return buf.size() * 8 + allocation_buf_usage; }
	int byteLength() { return buf.size() + (allocation_buf_usage ? 1 : 0); }
};