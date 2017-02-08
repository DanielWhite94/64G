#ifndef ENGINE_PHYSICS_HITMASK_H
#define ENGINE_PHYSICS_HITMASK_H

#include <cstdint>

namespace Engine {
	namespace Physics {
		class HitMask {
		public:
			HitMask();
			HitMask(uint64_t mask);
			~HitMask();

			bool getXY(unsigned x, unsigned y) const; // 0<=x,y<8

			void setXY(unsigned x, unsigned y, bool value); // 0<=x,y<8
		private:
			uint64_t bitset;

		};
	};
};

#endif
