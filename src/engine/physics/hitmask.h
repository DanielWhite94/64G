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

			void setColumn(int x, bool value);
			void setRow(int y, bool value);
			void setXY(unsigned x, unsigned y, bool value); // 0<=x,y<8

			void translateXY(int dX, int dY);
			void translateDown(unsigned n);
			void translateLeft(unsigned n);
			void translateRight(unsigned n);
			void translateUp(unsigned n);

			HitMask &operator|=(const HitMask &rhs) {
				this->bitset|=rhs.bitset;
				return *this;
			}

			friend HitMask operator|(HitMask lhs, const HitMask &rhs) {
				lhs|=rhs;
				return lhs;
			}

		private:
			static const uint64_t row0Mask=0x00000000000000FFllu;
			static const uint64_t column0Mask=0x0101010101010101llu;
			static const uint64_t column7Mask=0x8080808080808080llu;

			uint64_t bitset;
		};
	};
};

#endif
