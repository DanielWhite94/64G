#include <cassert>
#include <cstdlib>

#include "hitmask.h"

using namespace Engine;

namespace Engine {
	namespace Physics {
		HitMask::HitMask() {
			bitset=0;
		}

		HitMask::HitMask(uint64_t mask) {
			bitset=mask;
		}

		HitMask::HitMask(const char *str) {
			assert(str!=NULL);

			bitset=0;

			unsigned x=0, y=0;
			const char *c;
			for(c=str; *c!='\0'; ++c)
				switch(*c) {
					case '#':
						setXY(x, y, true);
						x=(x+1)%8;
						y+=(x==0);
					break;
					case '_':
						setXY(x, y, false);
						x=(x+1)%8;
						y+=(x==0);
					break;
				}
		}

		HitMask::~HitMask() {
		}

		bool HitMask::getXY(unsigned x, unsigned y) const  {
			const unsigned shift=x+y*8;
			return (bitset>>shift)&1;
		}

		uint64_t HitMask::getBitset(void) const {
			return bitset;
		}

		void HitMask::setColumn(int x, bool value) {
			if (x<0 || x>=8)
				return;

			HitMask column0=HitMask(column0Mask);
			column0.translateRight(x);
			if (value)
				bitset|=column0.bitset;
			else
				bitset&=~column0.bitset;
		}

		void HitMask::setRow(int y, bool value) {
			if (y<0 || y>=8)
				return;

			HitMask row0=HitMask(row0Mask);
			row0.translateDown(y);
			if (value)
				bitset|=row0.bitset;
			else
				bitset&=~row0.bitset;
		}

		void HitMask::setXY(unsigned x, unsigned y, bool value) {
			const unsigned shift=x+y*8;
			bitset&=~(((uint64_t)1)<<shift);
			bitset|=(((uint64_t)value)<<shift);
		}

		void HitMask::translateXY(int dX, int dY) {
			if (dX<0)
				translateLeft(-dX);
			else if (dX>0)
				translateRight(dX);

			if (dY<0)
				translateUp(-dY);
			else if (dY>0)
				translateDown(dY);
		}

		void HitMask::translateDown(unsigned n) {
			if (n>=8)
				bitset=0;
			else
				bitset<<=(n*8);
		}

		void HitMask::translateLeft(unsigned n) {
			if (n>=8)
				bitset=0;
			else
				while(n-->0)
					bitset=((bitset>>1) & ~column7Mask);
		}

		void HitMask::translateRight(unsigned n) {
			if (n>=8)
				bitset=0;
			else
				while(n-->0)
					bitset=((bitset<<1) & ~column0Mask);
		}

		void HitMask::translateUp(unsigned n) {
			if (n>=8)
				bitset=0;
			else
				bitset>>=(8*n);
		}

		void HitMask::flipHorizontally(void) {
			const uint64_t k1=0x5555555555555555llu;
			const uint64_t k2=0x3333333333333333llu;
			const uint64_t k4=0x0f0f0f0f0f0f0f0fllu;
			bitset=((bitset>>1)&k1)|((bitset&k1)<<1);
			bitset=((bitset>>2)&k2)|((bitset&k2)<<2);
			bitset=((bitset>>4)&k4)|((bitset&k4)<<4);
		}
	};
};
