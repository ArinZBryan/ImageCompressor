#include <deque>
#include <stdint.h>

class bitdeque
{
private:
	uint8_t backBuffer;
	std::deque<uint8_t> mainBuffer;
	uint8_t frontBuffer;

	int backBufferUsage;
	int frontBufferUsage;

public:
	//use after push_back or before take_front / pop_front
	bitdeque* flush_forward() {					
		if (backBufferUsage >= 8) {
			mainBuffer.push_back(backBuffer);
			backBuffer = 0x0;
			backBufferUsage = 0;
		}
		if (frontBuffer = 0) {
			frontBuffer = mainBuffer.front();
			mainBuffer.pop_front();
			frontBuffer = 8;
		}
		return this;
	}
	//use after push_front or before take_back / pop_back
	bitdeque* flush_backward() {
		if (frontBuffer >= 8) {
			mainBuffer.push_front(frontBuffer);
			frontBuffer = 0x0;
			frontBuffer = 0;
		}
		if (backBufferUsage = 0) {
			backBuffer = mainBuffer.back();
			mainBuffer.pop_back();
			backBufferUsage = 8;
		}
		return this;
	}
	
	bitdeque* push_back(bool value) {
		if (backBufferUsage < 8) {	//there is space in the backBuffer to avoid a flush
			backBuffer <<= 1;
			backBuffer |= value;
			backBufferUsage++;
		}
		else {						//the backBuffer is full
			flush_forward();
			backBuffer = value;
			backBuffer = 1;
		}
		return this;
	}
	bitdeque* push_front(bool value) {
		if (frontBufferUsage < 8) {	//there is space in the front buffer to avoid a flush
			frontBuffer >>= 1;
			frontBuffer |= (value << 7);
			frontBufferUsage++;
		}
		else {					//the inBuffer is full
			flush_backward();
			frontBuffer = value << 7;
			frontBufferUsage = 1;
		}
		return this;
	}
	bool pop_back() {
		flush_backward();
		bool ret = backBuffer & 0x1;
		backBuffer >> 1;
		backBufferUsage--;
		return ret;
	}
	bool pop_front() {
		flush_forward();
		bool ret = frontBuffer & 0x80;
		frontBuffer << 1;
		frontBufferUsage--;
		return ret;
	}
	uint32_t take_back(int n) {
		if (n > 32) { n = 32; }
		uint32_t ret = 0;
		for (int i = n - 1; i >= 0; i--) {
			ret |= pop_back() << i;
		}
		return ret;
	}
	uint32_t take_front(int n) {
		if (n > 32) { n = 32; }
		uint32_t ret = 0;
		for (int i = n - 1; i >= 0; i--) {
			ret |= pop_front() >> i;
		}
		return ret;
	}
};

