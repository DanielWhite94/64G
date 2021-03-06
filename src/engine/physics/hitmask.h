#ifndef ENGINE_PHYSICS_HITMASK_H
#define ENGINE_PHYSICS_HITMASK_H

#include <cstdint>

namespace Engine {
	namespace Physics {
		class HitMask {
		public:
			static const uint64_t emptyMask=0;
			static const uint64_t fullMask=UINT64_MAX;

			HitMask();
			HitMask(uint64_t mask);
			HitMask(const char *str);
			~HitMask();

			bool getXY(unsigned x, unsigned y) const; // 0<=x,y<8
			uint64_t getBitset(void) const;

			void setColumn(int x, bool value);
			void setRow(int y, bool value);
			void setXY(unsigned x, unsigned y, bool value); // 0<=x,y<8

			void translateXY(int dX, int dY);
			void translateDown(unsigned n);
			void translateLeft(unsigned n);
			void translateRight(unsigned n);
			void translateUp(unsigned n);

			void flipHorizontally(void);

			bool intersectsHitMask(const HitMask &hitMask) {
				return (getIntersectionWithHitMask(hitMask).bitset!=0);
			}

			HitMask getIntersectionWithHitMask(const HitMask &hitMask) {
				return HitMask(bitset & hitMask.bitset);
			}

			HitMask &operator&=(const HitMask &rhs) {
				this->bitset&=rhs.bitset;
				return *this;
			}

			friend HitMask operator&(HitMask lhs, const HitMask &rhs) {
				lhs&=rhs;
				return lhs;
			}

			HitMask &operator|=(const HitMask &rhs) {
				this->bitset|=rhs.bitset;
				return *this;
			}

			friend HitMask operator|(HitMask lhs, const HitMask &rhs) {
				lhs|=rhs;
				return lhs;
			}

			friend HitMask operator~(HitMask rhs) {
				HitMask lhs(~rhs.bitset);
				return lhs;
			}

			explicit operator bool() const {
				return (bitset!=0);
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
