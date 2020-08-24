#include <cassert>

#include "prng.h"

namespace Engine {
	Prng::Prng(unsigned seed): state(seed) {
	}

	Prng::~Prng() {
	}

	uint64_t Prng::gen64(void) {
		// lehmer64
		state*=0xda942042e4dd58b5;
		return state>>64;
	}

	uint64_t Prng::genN(uint64_t n) {
		assert(n>0);

		return gen64()%n;
	}
};
