#ifndef ENGINE_GEN_FLOODFILL_H
#define ENGINE_GEN_FLOODFILL_H

#include "../map/map.h"

namespace Engine {
	namespace Gen {
		bool floodFillBitsetNBoundaryFunctor(class Map *map, unsigned x, unsigned y, void *userData); // Returns the value of the nth bit in the tile's bitset, where n is (unsigned)(uintptr_t)userData.

		class FloodFill {
		public:
			// This class provides algorithms to flood-fill regions/groups of tiles based on a boolean
			// function applied to each tile to determine if it is a boundary.
			// Note: boundary tiles themselves are not filled.

			// This should return true if tile is considered to be a boundary tile, false otherwise
			// (e.g. if filling continents then traced continent edge tiles should return true and all other tiles should return false)
			typedef bool (BoundaryFunctor)(class Map *map, unsigned x, unsigned y, void *userData);

			// This is called for each tile which is filled
			// groupId is an integer which is unique to the group which contains this tile.
			typedef void (FillFunctor)(class Map *map, unsigned x, unsigned y, unsigned groupId, void *userData);

			typedef void (ProgressFunctor)(double progress, Util::TimeMs elapsedTimeMs, void *userData);

			// scratchBit should contain a unique tile bitset index which can be used freely by the fill algorithm internally
			FloodFill(class Map *map, unsigned scratchBit): map(map), scratchBit(scratchBit) {
			};
			~FloodFill() {};

			void fill(BoundaryFunctor *boundaryFunctor, void *boundaryUserData, FillFunctor *fillFunctor, void *fillUserData, ProgressFunctor *progressFunctor, void *progressUserData);
		private:
			class Map *map;

			unsigned scratchBit;
		};

	};
};

#endif
