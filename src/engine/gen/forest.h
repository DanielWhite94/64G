#ifndef ENGINE_GEN_FOREST_H
#define ENGINE_GEN_FOREST_H

#include "../map/map.h"

namespace Engine {
	namespace Gen {
		typedef bool (AddForestFunctor)(class Map *map, const CoordVec &position, void *userData);

		// Call a functor on a set of tiles in a region representing a random forest with fixed density.
		void addForest(class Map *map, const CoordVec &topLeft, const CoordVec &widthHeight, const CoordVec &interval, AddForestFunctor *functor, void *functorUserData);
	};
};

#endif
