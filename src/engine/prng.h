#ifndef ENGINE_PRNG_H
#define ENGINE_PRNG_H

#include <cstdint>

namespace Engine {
	class Prng {
	public:
		Prng(unsigned seed);
		~Prng();

		uint64_t gen64(void); // 0<gen64()<2^n
		uint64_t genN(uint64_t n); // 0<=genN(n)<n

	private:
		__uint128_t state;
	};
};

#endif
