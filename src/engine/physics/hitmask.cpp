#include <cstdlib>

#include "hitmask.h"

using namespace Engine;

namespace Engine {
	namespace Physics {
		HitMask::HitMask() {
			bitset=0;
		}

		HitMask::~HitMask() {
		}

		bool HitMask::getXY(unsigned x, unsigned y) const  {
			return (bitset>>(x+y*8))&1;
		}

		void HitMask::setXY(unsigned x, unsigned y, bool value) {
			unsigned shift=x+y*8;
			bitset&=~(((uint64_t)1)<<shift);
			bitset|=(((uint64_t)value)<<shift);
		}

	};
};
