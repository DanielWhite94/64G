#include <cassert>

#include "forest.h"

using namespace Engine;

namespace Engine {
	namespace Gen {
		void addForest(class Map *map, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, AddForestFunctor *functor, void *functorUserData) {
			assert(map!=NULL);
			assert(widthHeight.x>=0 && widthHeight.y>=0);
			assert(interval.x>0 && interval.y>0);
			assert(functor!=NULL);

			// Loop over rectangular region.
			CoordVec pos;
			for(pos.y=topLeft.y; pos.y<topLeft.y+widthHeight.y; pos.y+=interval.y)
				for(pos.x=topLeft.x; pos.x<topLeft.x+widthHeight.x; pos.x+=interval.x) {
					// Attempt to place object in this region.
					unsigned i;
					for(i=0; i<4; ++i) {
						// Calculate exact position.
						CoordVec randomOffset=CoordVec(Util::randIntInInterval(0,interval.x), Util::randIntInInterval(0,interval.y));
						CoordVec exactPosition=pos+randomOffset;

						// Run functor.
						if (functor(map, exactPosition, functorUserData))
							break;
					}
				}
		}
	};
};
