#pragma once

namespace th {

	class xorshf96 {
	public:
		unsigned long operator()() {
			unsigned long t;

			x ^= x << 16;
			x ^= x >> 5;
			x ^= x << 1;

			t = x;
			x = y;
			y = z;

			z = t ^ x ^ y;
			return z;
		}
	private:
		unsigned long x=123456789, y=362436069, z=521288629;
	};

}
